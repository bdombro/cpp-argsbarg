#pragma once

/// Declarations for schema validation and argv parsing (`parse.hpp` + `detail/parse_inline.hpp`).
///
/// Goal: separate the stable API surface from the large inline parser implementation.
/// Why: faster compiles for TUs that only need `ParseResult` types; tests can include parse only.
/// How: declares `schema_validate`, `parse`, `post_parse_validate`; definitions are included below.

#include "argsbarg/result.hpp"
#include "argsbarg/schema.hpp"

#include <string>
#include <vector>

namespace argsbarg {

/// Throws `SchemaError` if the merged schema tree is inconsistent (duplicate names, bad fallback).
void schema_validate(const Schema& merged);

/// Token-level parse: routes commands, consumes options, may yield Help/Error without I/O.
[[nodiscard]] ParseResult parse(const Schema& merged, const std::vector<std::string>& argv);

/// Coerces option strings to numbers where needed; final validation after `parse`.
[[nodiscard]] ParseResult post_parse_validate(const Schema& merged, ParseResult pr);

} // namespace argsbarg

#include "argsbarg/detail/parse_inline.hpp"
