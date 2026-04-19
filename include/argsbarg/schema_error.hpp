#pragma once

/// Exception type for invalid CLI schemas (duplicate names, illegal fallback, etc.).
///
/// Goal: distinguish author mistakes from user argv mistakes at the type level.
/// Why: `schema_validate` must fail fast before any user input is trusted.
/// How: thin subclass of `std::logic_error` thrown from validation and parser setup.

#include <stdexcept>

namespace argsbarg {

class SchemaError : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

} // namespace argsbarg
