/*
 * Copyright 2021-2023 Hewlett Packard Enterprise Development LP
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "core-types.h"
#include "chpl/uast/all-uast.h"
#include "python-types.h"

/* In this file are generated method and struct definitions for each AST node using
   the X-Macros pattern. Each 'include' of a header file, either uast-classes-list.h
   or method-tables.h, is used to generate a bunch of similar code for many
   classes. */

/* First, generate a Node_init for each type of node in the Dyno AST, 
   implemented in the DEFINE_INIT_FOR macro.

   We particularly want this to make sure we call the AstNode constructor,
   which sets the context object etc.
 */
#define DEFINE_INIT_FOR(NAME, TAG)\
  int NAME##Object_init(NAME##Object* self, PyObject* args, PyObject* kwargs) { \
    return parentTypeFor(chpl::uast::asttags::TAG)->tp_init((PyObject*) self, args, kwargs); \
  } \

/* Use the X-macros pattern to invoke DEFINE_INIT_FOR for each AST node type. */
#define AST_NODE(NAME) DEFINE_INIT_FOR(NAME, NAME)
#define AST_LEAF(NAME) DEFINE_INIT_FOR(NAME, NAME)
#define AST_BEGIN_SUBCLASSES(NAME) DEFINE_INIT_FOR(NAME, START_##NAME)
#define AST_END_SUBCLASSES(NAME)
#include "chpl/uast/uast-classes-list.h"
#undef AST_NODE
#undef AST_LEAF
#undef AST_BEGIN_SUBCLASSES
#undef AST_END_SUBCLASSES
#undef DEFINE_INIT_FOR

static const char* blockStyleToString(chpl::uast::BlockStyle blockStyle) {
  switch (blockStyle) {
    case chpl::uast::BlockStyle::EXPLICIT: return "explicit";
    case chpl::uast::BlockStyle::IMPLICIT: return "implicit";
    case chpl::uast::BlockStyle::UNNECESSARY_KEYWORD_AND_BLOCK: return "unnecessary";
  }
}

static const char* opKindToString(chpl::uast::Range::OpKind kind) {
  switch (kind) {
    case chpl::uast::Range::DEFAULT: return "..";
    case chpl::uast::Range::OPEN_HIGH: return "..<";
    default: return "";
  }
}

template<typename IntentType>
static const char* intentToString(IntentType intent) {
  return chpl::uast::qualifierToString(chpl::uast::Qualifier(int(intent)));
}

/* The METHOD macro is overridden here to actually create a Python-compatible
   function to insert into the method table. Each such function retrieves
   a node's context object, calls the method body, and wraps the result
   in a Python-compatible type.
 */
#define METHOD(NODE, NAME, DOCSTR, TYPEFN, BODY)\
  static PyObject* NODE##Object_##NAME(PyObject *self, PyObject *Py_UNUSED(ignored)) {\
    using namespace chpl; \
    using namespace uast; \
    \
    auto cast = ((NODE##Object*) self)->parent.astNode->to##NODE(); \
    auto contextObject = (ContextObject*) ((NODE##Object*) self)->parent.contextObject;\
    auto result = [](const NODE* node) { \
      BODY; \
    }(cast) ; \
    return PythonFnHelper<TYPEFN>::ReturnTypeInfo::wrap(contextObject, std::move(result));\
  }

/* Call METHOD on each method in the method-tables.h header to generate
   the Node_method(...) functions. */
#include "method-tables.h"

/* Helper macro to set up actual iterators. Needs to be a macro because nodes
   that have actuals don't all share a parent class (Attribute vs FnCall, e.g.).
   */
#define ACTUAL_ITERATOR(NAME)\
  static PyObject* NAME##Object_actuals(PyObject *self, PyObject *Py_UNUSED(ignored)) { \
    auto node = ((NAME##Object*) self)->parent.astNode->to##NAME(); \
    \
    auto argList = Py_BuildValue("(O)", (PyObject*) self); \
    auto astCallIterObjectPy = PyObject_CallObject((PyObject *) &AstCallIterType, argList); \
    Py_XDECREF(argList); \
    \
    auto astCalliterObject = (AstCallIterObject*) astCallIterObjectPy; \
    astCalliterObject->current = 0; \
    astCalliterObject->num = node->numActuals(); \
    astCalliterObject->container = node; \
    astCalliterObject->childGetter = [](const void* n, int child) { \
      return ((chpl::uast::NAME*) n)->actual(child); \
    }; \
    astCalliterObject->nameGetter = [](const void* n, int child) { \
      return ((chpl::uast::NAME*) n)->actualName(child); \
    }; \
    \
    return astCallIterObjectPy; \
  }

ACTUAL_ITERATOR(Attribute);
ACTUAL_ITERATOR(FnCall);

/* The following code is used to help set up the (Python) method tables for
   each Chapel AST node class. Each node class needs a table, but not all
   classes have Python methods we want to expose. We thus want to default
   to an empty method table (to save on typing / boilerplate), but at the same
   time to make it easy to override a node's method table.

   To this end, we use template specialization of the PerNodeInfo struct. The
   default template provides an empty table; it can be specialized per-node to
   change the table for that node.

   Macros below take this a step further and compiler-generate the template
   specializations. */
template <chpl::uast::asttags::AstTag tag>
struct PerNodeInfo {
  static constexpr PyMethodDef methods[] = {
    {NULL, NULL, 0, NULL}  /* Sentinel */
  };
};

#define CLASS_BEGIN(TAG) \
  template <> \
  struct PerNodeInfo<chpl::uast::asttags::TAG> { \
    static constexpr PyMethodDef methods[] = {
#define CLASS_END(TAG) \
      {NULL, NULL, 0, NULL}  /* Sentinel */ \
    }; \
  };
#define METHOD(NODE, NAME, DOCSTR, TYPE, BODY) \
  {#NAME, NODE##Object_##NAME, METH_NOARGS, DOCSTR},
#define METHOD_PROTOTYPE(NODE, NAME, DOCSTR) \
  {#NAME, NODE##Object_##NAME, METH_NOARGS, DOCSTR},
#include "method-tables.h"

/* Having generated the method calls and the method tables, we can now
   generate the Python type objects for each AST node. The DEFINE_PY_TYPE_FOR
   macro defines what a type object for an AST node (abstract or not) should
   look like. */

#define DEFINE_PY_TYPE_FOR(NAME, TAG, FLAGS)\
  PyTypeObject NAME##Type = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    .tp_name = #NAME, \
    .tp_basicsize = sizeof(NAME##Object), \
    .tp_itemsize = 0, \
    .tp_flags = FLAGS, \
    .tp_doc = PyDoc_STR("A Chapel " #NAME " AST node"), \
    .tp_methods = (PyMethodDef*) PerNodeInfo<TAG>::methods, \
    .tp_base = parentTypeFor(TAG), \
    .tp_init = (initproc) NAME##Object_init, \
    .tp_new = PyType_GenericNew, \
  }; \

/* Now, invoke DEFINE_PY_TYPE_FOR for each AST node to get our type objects. */
#define AST_NODE(NAME) DEFINE_PY_TYPE_FOR(NAME, chpl::uast::asttags::NAME, Py_TPFLAGS_DEFAULT)
#define AST_LEAF(NAME) DEFINE_PY_TYPE_FOR(NAME, chpl::uast::asttags::NAME, Py_TPFLAGS_DEFAULT)
#define AST_BEGIN_SUBCLASSES(NAME) DEFINE_PY_TYPE_FOR(NAME, chpl::uast::asttags::START_##NAME, Py_TPFLAGS_BASETYPE)
#define AST_END_SUBCLASSES(NAME)
#include "chpl/uast/uast-classes-list.h"
#undef AST_NODE
#undef AST_LEAF
#undef AST_BEGIN_SUBCLASSES
#undef AST_END_SUBCLASSES
