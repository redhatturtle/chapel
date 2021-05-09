/*
 * Copyright 2021 Hewlett Packard Enterprise Development LP
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

#include "chpl/frontend/Parser.h"
#include "chpl/queries/Context.h"
#include "chpl/uast/Call.h"
#include "chpl/uast/Expression.h"
#include "chpl/uast/FnCall.h"
#include "chpl/uast/Formal.h"
#include "chpl/uast/FormalDecl.h"
#include "chpl/uast/Function.h"
#include "chpl/uast/FunctionDecl.h"
#include "chpl/uast/Identifier.h"
#include "chpl/uast/ModuleDecl.h"
#include "chpl/uast/OpCall.h"

// always check assertions in this test
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

using namespace chpl;
using namespace uast;
using namespace frontend;

static void test1(Parser* parser) {
  auto parseResult = parser->parseString("test1.chpl",
                                         "proc f() { }");
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->name().compare("test1") == 0);
  assert(module->numStmts() == 1);
  auto functionDecl = module->stmt(0)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->name().compare("f") == 0);
  assert(functionDecl->linkage() == Function::DEFAULT_LINKAGE);
  assert(functionDecl->kind() == Function::PROC);
  assert(functionDecl->returnIntent() == Function::DEFAULT_RETURN_INTENT);
  assert(functionDecl->isInline() == false);
  assert(functionDecl->isOverride() == false);
  assert(functionDecl->throws() == false);
  assert(functionDecl->linkageNameExpression() == nullptr);
  assert(functionDecl->numFormals() == 0);
  assert(functionDecl->thisFormal() == nullptr);
  assert(functionDecl->returnType() == nullptr);
  assert(functionDecl->numLifetimeClauses() == 0);
  assert(functionDecl->numStmts() == 0);
}
static void test1a(Parser* parser) {
  auto parseResult = parser->parseString("test1a.chpl",
                                         "/* comment 1 */\n"
                                         "proc f() {\n"
                                         "  /* comment 2 */\n"
                                         "}\n"
                                         "/* comment 3 */\n");
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->name().compare("test1a") == 0);
  assert(module->numStmts() == 3);
  assert(module->stmt(0)->isComment());
  assert(module->stmt(1)->isFunctionDecl());
  assert(module->stmt(2)->isComment());
  auto functionDecl = module->stmt(1)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->numStmts() == 1);
  assert(functionDecl->stmt(0)->isComment());
}
static void test1b(Parser* parser) {
  auto parseResult = parser->parseString("test1a.chpl",
                                         "/* comment 1 */\n"
                                         "proc f() {\n"
                                         "  /* comment 2 */\n"
                                         "  x;\n"
                                         "  /* comment 3 */\n"
                                         "}\n"
                                         "/* comment 4 */\n");
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->name().compare("test1a") == 0);
  assert(module->numStmts() == 3);
  assert(module->stmt(0)->isComment());
  assert(module->stmt(1)->isFunctionDecl());
  assert(module->stmt(2)->isComment());
  auto functionDecl = module->stmt(1)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->numStmts() == 3);
  assert(functionDecl->stmt(0)->isComment());
  assert(functionDecl->stmt(1)->isIdentifier());
  assert(functionDecl->stmt(0)->isComment());
}

static void test1c(Parser* parser) {
  auto parseResult = parser->parseString("test1c.chpl",
                                         "/* comment 1 */\n"
                                         "public inline\n"
                                         "proc f() {\n"
                                         "  x;\n"
                                         "}\n");
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->name().compare("test1c") == 0);
  assert(module->numStmts() == 2);
  assert(module->stmt(0)->isComment());
  assert(module->stmt(1)->isFunctionDecl());
  auto functionDecl = module->stmt(1)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->numStmts() == 1);
  assert(functionDecl->stmt(0)->isIdentifier());
}

static void test1d(Parser* parser) {
  auto parseResult = parser->parseString("test1d.chpl",
                                         "public\n"
                                         "/* comment 1 */\n"
                                         "inline\n"
                                         "proc f() {\n"
                                         "  x;\n"
                                         "}\n");
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->name().compare("test1d") == 0);
  assert(module->numStmts() == 1);
  assert(module->stmt(0)->isFunctionDecl());
  auto functionDecl = module->stmt(0)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->numStmts() == 1);
  assert(functionDecl->stmt(0)->isIdentifier());
}

static void test1e(Parser* parser) {
  auto parseResult = parser->parseString("test1e.chpl",
                                         "public inline /* comment 1 */\n"
                                         "proc f() {\n"
                                         "  x;\n"
                                         "}\n");
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->name().compare("test1e") == 0);
  assert(module->numStmts() == 1);
  assert(module->stmt(0)->isFunctionDecl());
  auto functionDecl = module->stmt(0)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->numStmts() == 1);
  assert(functionDecl->stmt(0)->isIdentifier());
}

static Builder::Result parseFunction(Parser* parser,
                                     const char* filename,
                                     const Function*& f,
                                     const char* contents) {
  auto parseResult = parser->parseString(filename, contents);
  assert(parseResult.errors.size() == 0);
  assert(parseResult.topLevelExpressions.size() == 1);
  assert(parseResult.topLevelExpressions[0]->isModuleDecl());
  auto module = parseResult.topLevelExpressions[0]->toModuleDecl()->module();
  assert(module->numStmts() == 1);
  auto functionDecl = module->stmt(0)->toFunctionDecl();
  assert(functionDecl);
  assert(functionDecl->function());
  f = functionDecl->function();
  return std::move(parseResult);
}

static void test2(Parser* parser) {
  const Function* function = nullptr;
  auto parse = parseFunction(
      parser,
      "test2.chpl",
      function,
      "inline proc f(a: int) ref { x; }");

  assert(function->name().compare("f") == 0);
  assert(function->linkage() == Function::DEFAULT_LINKAGE);
  assert(function->kind() == Function::PROC);
  assert(function->returnIntent() == Function::REF);
  assert(function->isInline() == true);
  assert(function->isOverride() == false);
  assert(function->throws() == false);
  assert(function->linkageNameExpression() == nullptr);
  assert(function->numFormals() == 1);
  auto formal = function->formal(0);
  assert(formal);
  assert(formal->intent() == Formal::DEFAULT_INTENT);
  assert(formal->name().compare("a") == 0);
  auto typeExpr = formal->typeExpression();
  assert(typeExpr);
  assert(typeExpr->isIdentifier());
  assert(typeExpr->toIdentifier()->name().compare("int") == 0);
  assert(formal->initExpression() == nullptr);
  assert(function->thisFormal() == nullptr);
  assert(function->returnType() == nullptr);
  assert(function->numLifetimeClauses() == 0);
  assert(function->numStmts() == 1);
  auto stmt = function->stmt(0);
  assert(stmt);
  assert(stmt->isIdentifier());
  assert(stmt->toIdentifier()->name().compare("x") == 0);
}

static void test3(Parser* parser) {
  const Function* function = nullptr;
  auto parse = parseFunction(
      parser,
      "test3.chpl",
      function,
      "override proc const R.f(ref a: int = b) const ref { }");
  assert(function->name().compare("f") == 0);
  assert(function->linkage() == Function::DEFAULT_LINKAGE);
  assert(function->kind() == Function::PROC);
  assert(function->returnIntent() == Function::CONST_REF);
  assert(function->isInline() == false);
  assert(function->isOverride() == true);
  assert(function->throws() == false);
  assert(function->linkageNameExpression() == nullptr);
  assert(function->numFormals() == 2); // 'this' and 'a'

  auto thisFormal = function->formal(0);
  assert(thisFormal);
  assert(thisFormal->intent() == Formal::CONST);
  assert(thisFormal->name().compare("this") == 0);
  auto thisTypeExpr = thisFormal->typeExpression();
  assert(thisTypeExpr);
  assert(thisTypeExpr->isIdentifier());
  assert(thisTypeExpr->toIdentifier()->name().compare("R") == 0);
  assert(thisFormal->initExpression() == nullptr);

  auto formal = function->formal(1);
  assert(formal);
  assert(formal->intent() == Formal::REF);
  assert(formal->name().compare("a") == 0);
  auto typeExpr = formal->typeExpression();
  assert(typeExpr);
  assert(typeExpr->isIdentifier());
  assert(typeExpr->toIdentifier()->name().compare("int") == 0);
  auto initExpr = formal->initExpression();
  assert(initExpr);
  assert(initExpr->isIdentifier());
  assert(initExpr->toIdentifier()->name().compare("b") == 0);

  assert(function->thisFormal() == thisFormal);
  assert(function->returnType() == nullptr);
  assert(function->numLifetimeClauses() == 0);
  assert(function->numStmts() == 0);
}



int main() {
  Context context;
  Context* ctx = &context;

  auto parser = Parser::build(ctx);
  Parser* p = parser.get();

  test1(p);
  test1a(p);
  test1b(p);
  test1c(p);
  test1d(p);
  test1e(p);
  test2(p);
  test3(p);

  return 0;
}
