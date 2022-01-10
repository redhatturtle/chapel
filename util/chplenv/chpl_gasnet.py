import chpl_compiler, chpl_comm_debug, chpl_comm_segment, chpl_comm_substrate
import chpl_home_utils, compiler_utils, third_party_utils
from utils import error, warning, memoize
import os

@memoize
def get_uniq_cfg_path():
    base_uniq_cfg = third_party_utils.default_uniq_cfg_path()
    if chpl_comm_debug.get() == 'debug':
        base_uniq_cfg += '-debug'
    substrate = chpl_comm_substrate.get()
    segment = chpl_comm_segment.get()
    return base_uniq_cfg + '/substrate-' + substrate + '/seg-' + segment


@memoize
def get_gasnet_pc_file():
    substrate = chpl_comm_substrate.get()
    pcfile = "gasnet-{0}-par.pc".format(substrate)
    return pcfile

# If we are using a PrgEnv environment, gasnet is built with
# PrgEnv-gnu even when CHPL_TARGET_COMPILER is llvm.
# The gasnet .pc file includes some GCC options in that event.
# This function filters these out.
#
# This is a workaround.
# TODO: Do we still need to switch to PrgEnv-gnu when building gasnet?
def filter_compile_args(args):
    compiler = chpl_compiler.get('target')
    is_prgenv = compiler_utils.target_compiler_is_prgenv(bypass_llvm=True)
    if compiler == 'llvm' and is_prgenv:
        # filter out compile arguments not starting with -D or -I
        ret = [ ]
        n = len(args)
        i = 0
        while i < n:
            s = args[i]
            if s.startswith('-D') or s.startswith('-I'):
                ret.append(s)
            if len(s) == 2 and i+1 < n:
                # if it was just -D or -I, add the next argument too
                i += 1
                ret.append(args[i])
            i += 1
        return ret
    else:
        # otherwise, just return the args the way they were
        return args

# returns 2-tuple of lists
#  (compiler_bundled_args, compiler_system_args)
@memoize
def get_compile_args():
    tup = third_party_utils.pkgconfig_get_bundled_compile_args(
                       'gasnet', get_uniq_cfg_path(), get_gasnet_pc_file())
    return (filter_compile_args(tup[0]), filter_compile_args(tup[1]))

# returns 2-tuple of lists
#  (linker_bundled_args, linker_system_args)
@memoize
def get_link_args():
    return third_party_utils.pkgconfig_get_bundled_link_args(
                       'gasnet', get_uniq_cfg_path(), get_gasnet_pc_file())
