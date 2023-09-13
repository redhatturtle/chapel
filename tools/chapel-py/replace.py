import argparse
import chapel
import chapel.core
from collections import defaultdict
from bisect import bisect_left
import os
import sys

class ReplacementContext:
    def __init__(self, path):
        # Build the line number -> file position map
        with open(path, "r") as file:
            self.content = file.read()
        self.lines = {1: 0}
        self.lines_back = {}
        line = 1
        for (i, char) in enumerate(self.content):
            self.lines_back[i] = line
            if char == '\n':
                line += 1
                self.lines[line] = i+1 # the next characrer is the start of the next line

    def get_file_idx(self, loc):
        (row, col) = loc
        return self.lines[row] + (col - 1)


    def get_node_range(self, node):
        loc = node.location()

        range_start = self.get_file_idx(loc.start())
        range_end = self.get_file_idx(loc.end())
        return (range_start, range_end)

    def get_node_exact_string(self, node):
        (range_start, range_end) = self.get_node_range(node)
        return self.content[range_start:range_end]

    def get_node_indent(node):
        (range_start, _) = self.get_node_range(node)
        return range_start - lines[lines_back[range_start]]

def rename_formals(rc, fn, renames):
    for child in fn:
        if not isinstance(child, chapel.core.Formal): continue

        name = child.name()
        if name not in renames: continue

        child_text = rc.get_node_exact_string(child)
        yield (child, child_text.replace(name, renames[name]))

def rename_named_actuals(rc, call, renames):
    for actual in call.actuals():
        if isinstance(actual, tuple):
            (name, actual) = actual
            if name not in renames: continue

            actual_text = rc.get_node_exact_string(actual)

            # TODO: but there's no node that includes the name = in its location...
            # yield (actual, actual_text.replace(name, renames[name]))
            yield from []

def do_replace(finder, ctx, filename):
    asts = ctx.parse(filename)
    rc = ReplacementContext(filename)
    new_content = rc.content

    # First, store all the replacements in a map; then, walk the tree in a
    # reverse-postorder traversal (child nodes in reverse order, then parent)
    # and apply the transformations.

    nodes_to_replace = {}
    for ast in asts:
        for (node, replace_with) in finder(rc, ast):
            nodes_to_replace[node.unique_id()] = replace_with

    def recurse(node):
        my_replace = None
        if node.unique_id() in nodes_to_replace:
            my_replace = nodes_to_replace[node.unique_id()]

        # If we don't have our own substitution, we can just propagate the
        # child substitutions.
        if my_replace is None:
            for child in reversed(list(node)):
                yield from recurse(child)
        # If it's not callable, it must be a string; we don't care about child
        # replacements, since our own target is constant.
        elif not callable(my_replace):
            (replace_from, replace_to) = rc.get_node_range(node)
            yield (replace_from, replace_to, my_replace)

        # We have a callable replacement, which means we should apply child
        # substitutions to our text and then call the replacement with that.
        else:
            (replace_from, replace_to) = rc.get_node_range(node)
            my_text = rc.get_node_exact_string(node)
            for child in reversed(list(node)):
                for (child_from, child_to, child_str) in recurse(child):
                    # Child is not inside this node, so it can be replaced as before
                    if child_from >= replace_to:
                        yield (child_from, child_to, child_str)

                    # Otherwise, child is inside, and we have to apply the patch
                    # to our own content before handing it to the transformer.
                    else:
                        child_from -= replace_from
                        child_to -= replace_from

                        my_text = my_text[:child_from] + child_str + my_text[child_to:]
            yield (replace_from, replace_to, my_replace(my_text))

    for ast in reversed(asts):
        for (replace_from, replace_to, replace_with) in recurse(ast):
            new_content = new_content[:replace_from] + replace_with + new_content[replace_to:]

    with open(filename, "w") as newfile:
        newfile.write(new_content)

# -------------------------
# Application-Specific Code
# -------------------------

def find_this_complete(rc, root):
    # pattern for x._()
    methodcall = [chapel.core.FnCall, ["?dot",  chapel.core.Dot, ["?ident", chapel.core.Identifier]], chapel.rest]

    for (node, variables) in chapel.each_matching(root, methodcall):
        if variables["dot"].field() == "complete" and variables["ident"].name() == "this":
            yield (node, 'init this')

def rename_x_y_to_a_b(rc, root):
    for (fn, _) in chapel.each_matching(root, chapel.core.Function):
        if fn.name() != "changeMe": continue

        yield from rename_formals(rc, fn, { "x": "a", "y": "b" })

    # pattern for x(...)
    fncall = [chapel.core.FnCall, ["?ident", chapel.core.Identifier], chapel.rest]
    for (call, variables) in chapel.each_matching(root, fncall):
        if variables["ident"].name() != "changeMe": continue

        yield from rename_named_actuals(rc, call, { "x": "a", "y": "b" })

def tag_all_nodes_assert_on_gpu(rc, root):
    # pattern for x()
    noargcall = [chapel.core.FnCall, ["?ident", chapel.core.Identifier]]

    for (foreach, _) in chapel.each_matching(root, chapel.core.Foreach):
        has_assert_on_gpu = False

        loop_body = list(foreach)[-1];
        for child in loop_body:
            variables = chapel.match_pattern(child, noargcall)
            if variables is not None and variables["ident"].name() == "assertOnGpu":
                has_assert_on_gpu = True
                yield (child, '')

        indent = rc.get_node_indent(foreach)
        if has_assert_on_gpu:
            yield (foreach, lambda text, i = indent: "@assertOnGpu\n" + (" " * i) + text)

def tag_aggregates_with_io_interfaces(rc, root):
    aggrs_to_change = defaultdict(lambda: set())
    names_to_tag = defaultdict(lambda: set())

    for (fn, _) in chapel.each_matching(root, chapel.core.Function):
        if not fn.is_method(): continue
        name = fn.name()

        if name == "serialize":
            tag = "writeSerializable"
        elif name == "deserialize":
            tag = "readDeserializable"
        elif name == "init":
            formal_names = []
            for child in fn:
                if not isinstance(child, chapel.core.Formal): continue
                if child.name() == "this": continue
                formal_names.append(child.name())

            if len(formal_names) >=2 and formal_names[-1] == "deserializer" and formal_names[-2] == "reader":
                tag = "initDeserializable"
            else:
                continue
        else:
            continue

        if fn.is_primary_method():
            aggrs_to_change[fn.parent().unique_id()].add(tag)
            continue

        this_receiver = fn.this_formal()
        names_to_tag[rc.get_node_exact_string(this_receiver)].add(tag)

    def build_tag_str(tags):
        if len(tags) == 3: return "serializable"

        # tags have a preferred order, so just use an if-else chain to make that work
        the_order = ["writeSerializable", "readDeserializable", "initDeserializable"]
        return ", ".join(t for t in the_order if t in tags)

    for (record, _) in chapel.each_matching(root, chapel.core.AggregateDecl):
        tags = set()
        if record.unique_id() in aggrs_to_change:
            tags |= aggrs_to_change[record.unique_id()]
        if record.name() in names_to_tag:
            tags |= names_to_tag[record.name()]

        if len(tags) == 0: continue

        tag_str = build_tag_str(tags)
        record_text = rc.get_node_exact_string(record)
        curlypos = record_text.find("{")
        colonpos = record_text.find(":")

        if colonpos >= 0 and colonpos < curlypos:
            new_text = record_text.replace(" {" , ", " + tag_str + " {" , 1)
        else:
            new_text = record_text.replace(record.name(), record.name() + " : " + tag_str, 1)

        yield (record, new_text)

def all_transforms(rc, root):
    # yield from find_this_complete(rc, root)
    # yield from rename_x_y_to_a_b(rc, root)
    # yield from tag_all_nodes_assert_on_gpu(rc, root)
    yield from tag_aggregates_with_io_interfaces(rc, root)

parser = argparse.ArgumentParser( prog='replace', description='A tool to search-and-replace Chapel expressions with others')
parser.add_argument('filenames', nargs='*')
args = parser.parse_args()

basenames = defaultdict(lambda: 0)
buckets = defaultdict(lambda: [])

for filename in args.filenames:
    filename = os.path.realpath(os.path.expandvars(filename))

    basename = os.path.basename(filename)
    bucket = basenames[basename]
    basenames[basename] += 1
    buckets[bucket].append(filename)

for bucket in buckets:
    print("Bucket {}".format(bucket), file=sys.stderr)
    ctx = chapel.core.Context()
    to_replace = buckets[bucket]

    for filename in to_replace:
        print(filename, file=sys.stderr)
        do_replace(all_transforms, ctx, filename)
