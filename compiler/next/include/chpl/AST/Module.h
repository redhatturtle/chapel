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

#ifndef CHPL_AST_MODULE_H
#define CHPL_AST_MODULE_H

#include "chpl/AST/Symbol.h"
#include "chpl/AST/Location.h"

namespace chpl {
namespace ast {

/**
  This class represents a module. For example:

  \rst
  .. code-block:: chapel

      module M { }
  \endrst

  contains a ModuleDecl that refers to a Module Symbol.
 */
class Module final : public Symbol {
 friend class Builder;
 friend class ModuleDecl;

 public:
  enum Tag {
    DEFAULT,
    PROTOTYPE,
    IMPLICIT,
  };

 private:
  Tag tag_;

  Module(ASTList children,
         UniqueString name, Symbol::Visibility vis,
         Module::Tag tag);
  bool contentsMatchInner(const BaseAST* other) const override;

 public:
  ~Module() override = default;
  const Tag tag() const { return this->tag_; }
  int numStmts() const {
    return this->numChildren();
  }
  const Expr* stmt(int i) const {
    const BaseAST* ast = this->child(i);
    assert(ast->isExpr());
    return (Expr*) ast;
  }
};

} // end namespace ast
} // end namespace chpl

#endif
