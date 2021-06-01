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

#ifndef CHPL_UAST_REALLITERAL_H
#define CHPL_UAST_REALLITERAL_H

#include "chpl/queries/Location.h"
#include "chpl/uast/Literal.h"

namespace chpl {
namespace uast {


/**
  This class represents a floating point literal that is not imaginary.
  That is, it is a "real" number. Examples include ``0.0``, and `3e24`.
 */
class RealLiteral final : public Literal {
 private:
  double value_;
  int base_;

  RealLiteral(double value, int base)
    : Literal(asttags::RealLiteral),
      value_(value),
      base_(base)
  { }
  bool contentsMatchInner(const ASTNode* other) const override;
  void markUniqueStringsInner(Context* context) const override;

 public:
  ~RealLiteral() override = default;

  static owned<RealLiteral> build(Builder* builder, Location loc,
                                  double value, int base);

  /**
   Returns the value of this RealLiteral.
   */
  double value() const { return value_; }
 
  /**
   Returns the base of the number when it was parsed.
   */
  int base() const { return base_; }
};


} // end namespace uast
} // end namespace chpl

#endif
