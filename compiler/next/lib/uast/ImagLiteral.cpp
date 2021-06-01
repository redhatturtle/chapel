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

#include "chpl/uast/ImagLiteral.h"

#include "chpl/uast/Builder.h"

#include <cstdlib>

namespace chpl {
namespace uast {


bool ImagLiteral::contentsMatchInner(const ASTNode* other) const {
  const ImagLiteral* lhs = this;
  const ImagLiteral* rhs = (const ImagLiteral*) other;
  return lhs->literalContentsMatchInner(rhs) &&
         lhs->value_ == rhs->value_ &&
         lhs->base_ == rhs->base_;
}
void ImagLiteral::markUniqueStringsInner(Context* context) const {
  literalMarkUniqueStringsInner(context);
}

owned<ImagLiteral> ImagLiteral::build(Builder* builder, Location loc,
                                      double value, int base) {
  ImagLiteral* ret = new ImagLiteral(value, base);
  builder->noteLocation(ret, loc);
  return toOwned(ret);
}


} // namespace uast
} // namespace chpl
