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

#include "test-resolution.h"
#include "test-minimal-modules.h"

#include "chpl/parsing/parsing-queries.h"
#include "chpl/resolution/resolution-queries.h"
#include "chpl/resolution/scope-queries.h"
#include "chpl/types/all-types.h"
#include "chpl/uast/Identifier.h"
#include "chpl/uast/Module.h"
#include "chpl/uast/Record.h"
#include "chpl/uast/Variable.h"

static void testRectangular(std::string domainType,
                                  int rank,
                                  std::string idxType,
                                  bool stridable) {
  Context ctx;
  Context* context = &ctx;
  ErrorGuard guard(context);

  std::string program = DomainModule +
R"""(
module M {
  use ChapelDomain;
  
  var d : )""" + domainType + R"""(;
  param rg = )""" + std::to_string(rank) + R"""(;
  type ig = )""" + idxType + R"""(;
  type fullIndex = if rg == 1 then ig else rg*ig;

  param r = d.rank;
  type i = d.idxType;
  param s = d.stridable;
  param rk = d.isRectangular();
  param ak = d.isAssociative();

  var p = d.pid();

  for loopI in d {
    var z = loopI;
  }

  proc generic(arg: domain) {
    type GT = arg.type;
    return 42;
  }

  proc concrete(arg: )""" + domainType + R"""() {
    type CT = arg.type;
    return 42;
  }

  var g_ret = generic(d);
  var c_ret = concrete(d);
}
)""";

  auto path = UniqueString::get(context, "input.chpl");
  setFileText(context, path, std::move(program));

  const ModuleVec& vec = parseToplevel(context, path);
  const Module* m = vec[1]; 

  const ResolutionResultByPostorderID& rr = resolveModule(context, m->id());

  const Variable* d = findOnlyNamed(m, "d")->toVariable();
  QualifiedType dType = rr.byAst(d).type();

  const Variable* fullIndex = findOnlyNamed(m, "fullIndex")->toVariable();
  QualifiedType fullIndexType = rr.byAst(fullIndex).type();

  {
    const Variable* r = findOnlyNamed(m, "r")->toVariable();
    assert(rr.byAst(r).type().param()->toIntParam()->value() == rank);
  }

  {
    const Variable* ig = findOnlyNamed(m, "ig")->toVariable();
    const Variable* i = findOnlyNamed(m, "i")->toVariable();
    assert(rr.byAst(ig).type() == rr.byAst(i).type());
  }

  {
    const Variable* s = findOnlyNamed(m, "s")->toVariable();
    assert(rr.byAst(s).type().param()->toBoolParam()->value() == stridable);
  }

  {
    const Variable* rk = findOnlyNamed(m, "rk")->toVariable();
    assert(rr.byAst(rk).type().param()->toBoolParam()->value() == true);
  }

  {
    const Variable* ak = findOnlyNamed(m, "ak")->toVariable();
    assert(rr.byAst(ak).type().param()->toBoolParam()->value() == false);
  }

  {
    const Variable* p = findOnlyNamed(m, "p")->toVariable();
    assert(rr.byAst(p).type().type() == IntType::get(context, 0));
  }

  {
    const Variable* z = findOnlyNamed(m, "z")->toVariable();
    auto res = rr.byAst(z);
    assert(res.type().type() == fullIndexType.type());
  }

  {
    const Variable* g_ret = findOnlyNamed(m, "g_ret")->toVariable();
    auto res = rr.byAst(g_ret);
    assert(res.type().type()->isIntType());

    auto call = resolveOnlyCandidate(context, rr.byAst(g_ret->initExpression()));
    // Generic function, should have been instantiated
    assert(call->signature()->instantiatedFrom() != nullptr);

    const Variable* GT = findOnlyNamed(m, "GT")->toVariable();
    assert(call->byAst(GT).type().type() == dType.type());
  }

  {
    const Variable* c_ret = findOnlyNamed(m, "c_ret")->toVariable();
    auto res = rr.byAst(c_ret);
    assert(res.type().type()->isIntType());

    auto call = resolveOnlyCandidate(context, rr.byAst(c_ret->initExpression()));
    // Concrete function, should not be instantiated
    assert(call->signature()->instantiatedFrom() == nullptr);

    const Variable* CT = findOnlyNamed(m, "CT")->toVariable();
    assert(call->byAst(CT).type().type() == dType.type());
  }

  assert(guard.errors().size() == 0);

  printf("Success: %s\n", domainType.c_str());
}

static void testAssociative(std::string domainType,
                                  std::string idxType,
                                  bool parSafe) {
  Context ctx;
  Context* context = &ctx;
  ErrorGuard guard(context);

  std::string program = DomainModule +
R"""(
module M {
  use ChapelDomain;
  
  var d : )""" + domainType + R"""(;
  type ig = )""" + idxType + R"""(;

  type i = d.idxType;
  param s = d.parSafe;
  param rk = d.isRectangular();
  param ak = d.isAssociative();

  var p = d.pid();

  for loopI in d {
    var z = loopI;
  }

  proc generic(arg: domain) {
    type GT = arg.type;
    return 42;
  }

  proc concrete(arg: )""" + domainType + R"""() {
    type CT = arg.type;
    return 42;
  }

  var g_ret = generic(d);
  var c_ret = concrete(d);
}
)""";
  // TODO: generic checks

  auto path = UniqueString::get(context, "input.chpl");
  setFileText(context, path, std::move(program));

  const ModuleVec& vec = parseToplevel(context, path);
  const Module* m = vec[1]; 

  const ResolutionResultByPostorderID& rr = resolveModule(context, m->id());

  const Variable* d = findOnlyNamed(m, "d")->toVariable();
  QualifiedType dType = rr.byAst(d).type();

  const Variable* i = findOnlyNamed(m, "i")->toVariable();
  auto fullIndexType = rr.byAst(i).type();
  {
    const Variable* ig = findOnlyNamed(m, "ig")->toVariable();
    assert(rr.byAst(ig).type() == fullIndexType);
  }

  {
    const Variable* s = findOnlyNamed(m, "s")->toVariable();
    assert(rr.byAst(s).type().param()->toBoolParam()->value() == parSafe);
  }

  {
    const Variable* rk = findOnlyNamed(m, "rk")->toVariable();
    assert(rr.byAst(rk).type().param()->toBoolParam()->value() == false);
  }

  {
    const Variable* ak = findOnlyNamed(m, "ak")->toVariable();
    assert(rr.byAst(ak).type().param()->toBoolParam()->value() == true);
  }

  {
    const Variable* p = findOnlyNamed(m, "p")->toVariable();
    assert(rr.byAst(p).type().type() == IntType::get(context, 0));
  }

  {
    const Variable* z = findOnlyNamed(m, "z")->toVariable();
    auto res = rr.byAst(z);
    assert(res.type().type() == fullIndexType.type());
  }
  {
    const Variable* g_ret = findOnlyNamed(m, "g_ret")->toVariable();
    auto res = rr.byAst(g_ret);
    assert(res.type().type()->isIntType());

    auto call = resolveOnlyCandidate(context, rr.byAst(g_ret->initExpression()));
    // Generic function, should have been instantiated
    assert(call->signature()->instantiatedFrom() != nullptr);

    const Variable* GT = findOnlyNamed(m, "GT")->toVariable();
    assert(call->byAst(GT).type().type() == dType.type());
  }

  {
    const Variable* c_ret = findOnlyNamed(m, "c_ret")->toVariable();
    auto res = rr.byAst(c_ret);
    assert(res.type().type()->isIntType());

    auto call = resolveOnlyCandidate(context, rr.byAst(c_ret->initExpression()));
    // Concrete function, should not be instantiated
    assert(call->signature()->instantiatedFrom() == nullptr);

    const Variable* CT = findOnlyNamed(m, "CT")->toVariable();
    assert(call->byAst(CT).type().type() == dType.type());
  }

  assert(guard.errors().size() == 0);

  printf("Success: %s\n", domainType.c_str());
}

static void testBadPass(std::string argType, std::string actualType) {
  // Ensure that we can't, e.g.,  pass a domain(1) to a domain(2)
  Context ctx;
  Context* context = &ctx;
  ErrorGuard guard(context);

  std::string program = DomainModule +
R"""(
module M {
  use ChapelDomain;
  
  proc foo(arg: )""" + argType + R"""() {
    return 42;
  }

  var d : )""" + actualType + R"""(;
  var c_ret = foo(d);
}
)""";
  // TODO: generic checks

  auto path = UniqueString::get(context, "input.chpl");
  setFileText(context, path, std::move(program));

  const ModuleVec& vec = parseToplevel(context, path);
  const Module* m = vec[1]; 

  const ResolutionResultByPostorderID& rr = resolveModule(context, m->id());

  auto c_ret = findOnlyNamed(m, "c_ret")->toVariable();
  assert(rr.byAst(c_ret).type().isErroneousType());
  assert(guard.errors().size() == 1);
  auto& e = guard.errors()[0];
  assert(e->message() == "Cannot resolve call to 'foo': no matching candidates");

  printf("Success: cannot pass %s to %s\n", actualType.c_str(), argType.c_str());

  // 'clear' rather than 'realize' to simplify test output
  guard.clearErrors();
}

int main() {
  testRectangular("domain(1)", 1, "int", false);
  testRectangular("domain(2)", 2, "int", false);
  testRectangular("domain(1, stridable=true)", 1, "int", true);
  testRectangular("domain(2, int(8))", 2, "int(8)", false);
  testRectangular("domain(3, int(16), true)", 3, "int(16)", true);
  testRectangular("domain(stridable=false, idxType=int, rank=1)", 1, "int", false);

  testAssociative("domain(int)", "int", true);
  testAssociative("domain(int, false)", "int", false);
  testAssociative("domain(string)", "string", true);

  testBadPass("domain(1)", "domain(2)");
  testBadPass("domain(int)", "domain(string)");
  testBadPass("domain(1)", "domain(int)");

  return 0;
}
