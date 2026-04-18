#pragma once

#include <stdexcept>

namespace argsbarg {

class SchemaError : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

} // namespace argsbarg
