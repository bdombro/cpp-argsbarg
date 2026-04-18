#include <argsbarg/argsbarg.hpp>

#include <cstring>

int main() {
    const char* v = argsbarg::version();
    return (v != nullptr && std::strlen(v) >= 5) ? 0 : 1;
}
