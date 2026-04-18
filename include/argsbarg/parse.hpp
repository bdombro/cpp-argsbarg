#pragma once

#include "argsbarg/result.hpp"
#include "argsbarg/schema.hpp"

#include <string>
#include <vector>

namespace argsbarg {

void schema_validate(const Schema& merged);

[[nodiscard]] ParseResult parse(const Schema& merged, const std::vector<std::string>& argv);

[[nodiscard]] ParseResult post_parse_validate(const Schema& merged, ParseResult pr);

} // namespace argsbarg

#include "argsbarg/detail/parse_inline.hpp"
