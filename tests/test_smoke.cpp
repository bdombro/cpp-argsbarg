#include <argsbarg/argsbarg.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/wait.h>

using namespace argsbarg;

namespace {

int run_exe(const char* exe, const std::string& tail) {
    const std::string q = "'" + std::string{exe} + "'";
    return std::system((q + " " + tail + " >/dev/null 2>&1").c_str());
}

} // namespace

#ifndef ARGSBARG_TEST_MINIMAL_EXE
#error ARGSBARG_TEST_MINIMAL_EXE must be defined by CMake
#endif
#ifndef ARGSBARG_TEST_NESTED_EXE
#error ARGSBARG_TEST_NESTED_EXE must be defined by CMake
#endif

TEST_CASE("minimal example exit codes") {
    const char* exe = ARGSBARG_TEST_MINIMAL_EXE;
    REQUIRE(run_exe(exe, "hello") == 0);
    REQUIRE(run_exe(exe, "-h") == 0);
    // MissingOrUnknown fallback: bare argv runs default `hello` (exit 0), not implicit help.
    REQUIRE(run_exe(exe, "") == 0);
    REQUIRE(run_exe(exe, "not-a-command") != 0);
}

TEST_CASE("nested example implicit help on empty argv") {
    const char* exe = ARGSBARG_TEST_NESTED_EXE;
    REQUIRE(run_exe(exe, "") != 0);
}

TEST_CASE("minimal zsh completion print emits script") {
    const char* exe = ARGSBARG_TEST_MINIMAL_EXE;
    const std::string q = "'" + std::string{exe} + "'";
    const std::string cmd = q + " completion zsh --print | head -c 20 | wc -c";
    FILE* p = popen(cmd.c_str(), "r");
    REQUIRE(p != nullptr);
    char buf[32]{};
    std::fread(buf, 1, sizeof(buf) - 1, p);
    const int st = pclose(p);
    REQUIRE(WIFEXITED(st));
    REQUIRE(std::atoi(buf) > 10);
}

TEST_CASE("bash completion script syntax") {
    Schema s{.name = "demo", .description = ""};
    s.commands.push_back(Leaf{"hi", ""}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto script = completion_bash_script(m);
    const auto base = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
    const auto path = std::string{base} + "/argsbarg_bash_n_" +
                      std::to_string(reinterpret_cast<std::uintptr_t>(&script)) + ".sh";
    {
        std::ofstream out(path, std::ios::trunc | std::ios::binary);
        REQUIRE(out);
        out << script;
    }
    const std::string cmd = std::string{"bash -n "} + path + " 2>&1";
    const int rc = std::system(cmd.c_str());
    REQUIRE(rc == 0);
}
