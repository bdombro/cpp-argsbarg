import re
from pathlib import Path

from conan import ConanFile
from conan.errors import ConanException
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class ArgsbargConan(ConanFile):
    name = "argsbarg"
    package_type = "header-library"
    license = "MIT"
    url = "https://github.com/bdombro/cpp-argsbarg"
    homepage = "https://github.com/bdombro/cpp-argsbarg"
    description = (
        "Small C++23 header-only toolkit for CLIs with POSIX-style options, "
        "nested subcommands, and bash/zsh completion generation."
    )
    topics = ("cli", "command-line", "header-only", "cpp23", "args", "completion")

    settings = "os", "compiler", "build_type", "arch"

    exports_sources = (
        "CMakeLists.txt",
        "cmake/argsbargConfig.cmake.in",
        "include/argsbarg/*.hpp",
        "include/argsbarg/detail/*.hpp",
    )
    no_copy_source = True

    def set_version(self):
        hdr = Path(self.recipe_folder) / "include/argsbarg/argsbarg.hpp"
        if not hdr.is_file():
            raise ConanException(f"Missing version header: {hdr}")
        text = hdr.read_text(encoding="utf-8")
        m = re.search(r'return\s+"(\d+\.\d+\.\d+)"', text)
        if not m:
            raise ConanException("Could not parse semver from version() in argsbarg.hpp")
        self.version = m.group(1)

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, "23")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["ARGSBARG_BUILD_EXAMPLES"] = False
        tc.cache_variables["ARGSBARG_BUILD_TESTS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install(component="argsbarg_Development")

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.set_property("cmake_file_name", "argsbarg")
        self.cpp_info.set_property("cmake_target_name", "argsbarg::argsbarg")
