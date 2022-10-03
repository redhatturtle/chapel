/*
 * Copyright 2021-2022 Hewlett Packard Enterprise Development LP
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
#ifndef CHPL_FRAMEWORK_ERRORWRITER_H
#define CHPL_FRAMEWORK_ERRORWRITER_H

#include <ostream>

#include "chpl/framework/ErrorBase.h"
#include "chpl/uast/all-uast.h"
#include "chpl/parsing/parsing-queries.h"
#include "chpl/util/terminal.h"

namespace chpl {

class Context;

namespace errordetail {

/**
  Sometimes, locations reported using IDs are treated
  differently than locations reported using Location. In particular,
  the production compiler will print "In module: ..." if given an ID.

  This class is a shim for methods of ErrorWriterBase, which, when
  used as a location, results in a Location being produced. Thus,
  note(id, ...) will use an ID, whereas note(locationOnly(id),...) will
  use a Location.

  This class is used because error messages don't get access to
  the context themselves, and thus can't locate IDs themselves.
 */
template <typename T>
struct LocationOnly {
  T t; /* The thing whose location to compute */
};

inline Location locate(Context* context, const ID& id) {
  if (!context) return Location();
  return parsing::locateId(context, id);
}
inline Location locate(Context* context, const uast::AstNode* node) {
  return locate(context, node->id());
}

/// \cond DO_NOT_DOCUMENT
/**
  Wrapper class for any piece of data with a location that, when converted
  to a string, results in a printing the data's file and line number.
 */
template <typename T>
struct AsFileName {
  T t; /* The thing to locate */

  Location location(Context* context) const {
    return locate(context, t);
  }
};

/**
  Template to be partially specialized to provide custom to-string output
  specific to the ErrorWriter. Uses stringify by default.
 */
template <typename T>
struct Writer {
  void operator()(Context* context, std::ostream& oss, const T& t) {
    stringify<T> str;
    str(oss, CHPL_SYNTAX, t);
  }
};

template <>
struct Writer<const char*> {
  void operator()(Context* context, std::ostream& oss, const char* t) {
    oss << t;
  }
};

template <>
struct Writer<std::string> {
  void operator()(Context* context, std::ostream& oss, const std::string& t) {
    oss << t;
  }
};

template <typename T>
struct Writer<errordetail::AsFileName<T>> {
  void operator()(Context* context, std::ostream& oss, const errordetail::AsFileName<T>& e) {
    auto loc = e.location(context);
    oss << loc.path().c_str() << ":" << loc.firstLine();
  }
};
/// \endcond

} // end namespace errordetail

/**
  Helper function to create an AsFileName class.
 */
template <typename T>
errordetail::AsFileName<T> fileNameOf(T t) {
  return errordetail::AsFileName<T> { std::move(t) };
}

/**
  Helper function to create a LocationOnly class.
 */
template <typename T>
errordetail::LocationOnly<T> locationOnly(T t) {
  return errordetail::LocationOnly<T> { std::move(t) };
}

/**
  ErrorWriterBase is the main way for error messages to output diagnostic
  information. It abstracts away writing code to output streams (in fact,
  some instances of ErrorWriterBase do not write to a stream at all),
  and provides functionality like printing and underlining code. The way
  data is formatted when fed to the various printing functions like ::heading
  and ::message is specified by specializations of the errordetail::Writer
  class.

  The ErrorWriterBase expects that the ::heading function be called first
  by every error message; this function serves the double purpose of printing
  out the error heading, as well as setting the error message's location.
 */
class ErrorWriterBase {
 protected:
  Context* context;

  ErrorWriterBase(Context* context) : context(context) {}

  /**
    Write the error heading, possibly with some color and text decoration.
    The location given to this function and its overloads is considered
    the error's main location.
   */
  virtual void writeHeading(ErrorBase::Kind kind, Location loc, const std::string& message) = 0;
  virtual void writeHeading(ErrorBase::Kind kind, const ID& id, const std::string& message);
  virtual void writeHeading(ErrorBase::Kind kind, const uast::AstNode* ast, const std::string& message);
  template <typename T>
  void writeHeading(ErrorBase::Kind kind, errordetail::LocationOnly<T> t, const std::string& message) {
    writeHeading(kind, errordetail::locate(context, t.t), message);
  }

  /**
    Write additional information about the error. This information is only
    included when the error is reported in detailed mode; when printed
    in brief mode, information printed via writeMessage is omitted.
   */
  virtual void writeMessage(const std::string& message) = 0;

  /**
    Write a note about the error. Unlike messages, notes are printed
    even in brief mode. Thus, notes can be used to provide information
    that is useful "in all cases" (e.g., the location of a duplicate
    definition).
   */
  virtual void writeNote(Location loc, const std::string& message) = 0;
  virtual void writeNote(const ID& id, const std::string& message);
  virtual void writeNote(const uast::AstNode* ast, const std::string& message);
  template <typename T>
  void writeNote(errordetail::LocationOnly<T> t, const std::string& message) {
    writeNote(errordetail::locate(context, t.t), message);
  }

  /**
    Prints the lines of code associated with the given location. Additional
    locations provided via ::toHighlight field are underlined when the
    code is printed.
   */
  virtual void writeCode(const Location& place,
                         const std::vector<Location>& toHighlight = {}) = 0;

  template <typename ... Ts>
  std::string toString(Ts ... ts) {
    std::ostringstream oss;
    auto write = [&](auto t) {
      errordetail::Writer<decltype(t)> writer;
      writer(context, oss, t);
    };

    auto dummy = { (write(ts), 0)..., };
    (void) dummy;
    return oss.str();
  }
 public:
  /**
    Write the error heading, possibly with some color and text decoration.
    The location given to this function and its overloads is considered
    the error's main location.

    The variable arguments given to this function are automatically converted
    to strings.
   */
  template <typename LocationType, typename ... Ts>
  void heading(ErrorBase::Kind kind, LocationType loc, Ts ... ts) {
    writeHeading(kind, loc, toString(std::forward<Ts>(ts)...));
  }

  /**
    Write a note about the error. Unlike messages, notes are printed
    even in brief mode. Thus, notes can be used to provide information
    that is useful "in all cases" (e.g., the location of a duplicate
    definition).

    The variable arguments given to this function are automatically converted
    to strings.
   */
  template <typename ... Ts>
  void message(Ts ... ts) {
    writeMessage(toString(std::forward<Ts>(ts)...));
  }

  /**
    Write a note about the error. Unlike messages, notes are printed
    even in brief mode. Thus, notes can be used to provide information
    that is useful "in all cases" (e.g., the location of a duplicate
    definition).

    The variable arguments given to this function are automatically converted
    to strings.
   */
  template <typename LocationType, typename ... Ts>
  void note(LocationType loc, Ts ... ts) {
    writeNote(loc, toString(std::forward<Ts>(ts)...));
  }

  /**
    Prints the lines of code associated with the given location. Additional
    locations provided via ::toHighlight field are underlined when the
    code is printed.

    This function accepts any type for which location can be inferred,
    for both the main location and the highlights.
   */
  template <typename LocPlace, typename LocHighlight = const uast::AstNode*>
  void code(const LocPlace& place,
                const std::vector<LocHighlight>& toHighlight = {}) {
    std::vector<Location> ids(toHighlight.size());
    std::transform(toHighlight.cbegin(), toHighlight.cend(), ids.begin(), [&](auto node) {
      return errordetail::locate(context, node);
    });
    writeCode(errordetail::locate(context, place), ids);
  }

};

/**
  Implementation of ErrorWriterBase that prints error output to a stream.
  This class' output varies depending on if the error message is printed
  in brief or detailed mode, as well as if the useColor flag is set.
 */
class ErrorWriter : public ErrorWriterBase {
 public:
   /** The style of error reporting that the ErrorWriter should produce. */
  enum OutputFormat {
    /** Specify that all information about the error should be printed. */
    DETAILED,
    /** Specify that only key parts of the error should be printed. */
    BRIEF,
  };
 protected:
  std::ostream& oss_;
  OutputFormat outputFormat_;
  bool useColor_;

  void setColor(TermColorName color);

  void writeHeading(ErrorBase::Kind kind, Location loc,
                    const std::string& message) override;
  void writeMessage(const std::string& message) override {
    if (outputFormat_ == DETAILED) {
      oss_ << message << std::endl;
    }
  }
  void writeNote(Location loc, const std::string& message) override;
  void writeCode(const Location& place,
                 const std::vector<Location>& toHighlight = {}) override;
 public:
  ErrorWriter(Context* context, std::ostream& oss,
              OutputFormat outputFormat, bool useColor) :
    ErrorWriterBase(context), oss_(oss),
    outputFormat_(outputFormat), useColor_(useColor) {}

  void writeNewline();
};

} // end namespace chpl

#endif
