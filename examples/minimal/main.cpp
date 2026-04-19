#include <argsbarg/argsbarg.hpp>
#include <iostream>

using namespace argsbarg;

int main(int argc, const char* argv[]) {
    auto greet = [](Context& ctx) {
        const auto name = ctx.string_opt("name").value_or("world");
        if (ctx.flag("verbose")) {
            std::cout << "verbose mode\n";
        }
        std::cout << "hello " << name << '\n';
    };

    Application{"minimaldemo"}
        .description("Tiny demo.")
        .fallback("hello", FallbackMode::MissingOrUnknown)
        .command(Leaf{"hello", "Say hello."}
                     .handler(greet)
                     .option(Opt{"name", "Who to greet."}.string().short_alias('n'))
                     .option(Opt{"verbose", "Enable extra logging."}.short_alias('v')))
        .run(argc, argv);
}
