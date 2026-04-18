#include <argsbarg/argsbarg.hpp>
#include <fstream>
#include <iostream>
#include <string>

using namespace argsbarg;

namespace {

void read_cmd(Context& ctx) {
    const auto& files = ctx.args();
    if (files.empty()) {
        err_with_help(ctx, "Missing file path.");
    }
    for (const auto& f : files) {
        std::ifstream in(f);
        if (!in) {
            err_with_help(ctx, "Cannot open: " + f);
        }
        std::string line;
        if (std::getline(in, line)) {
            std::cout << f << ": " << line << '\n';
        }
    }
}

void stat_owner_lookup(Context& ctx) {
    const auto user = ctx.string_opt("user-name").value_or("?");
    const auto& files = ctx.args();
    if (files.empty()) {
        err_with_help(ctx, "Missing path.");
    }
    std::cout << "lookup user=" << user << " path=" << files.front() << '\n';
}

} // namespace

int main(int argc, const char* argv[]) {
    Application{"nesteddemo"}
        .description("Nested groups demo.")
        .fallback("read", FallbackMode::UnknownOnly)
        .command(Group{"stat", "File metadata."}.child(Group{"owner", "Ownership helpers."}.child(
            Leaf{"lookup", "Resolve owner info."}
                .handler(stat_owner_lookup)
                .option(Opt{"user-name", "User to look up."}.string().short_alias('u'))
                .arg(Arg{"path", "File or directory."}))))
        .command(Leaf{"read", "Print the first line of each file."}.handler(read_cmd).arg(
            Arg{"files", "Paths to read."}.min(1)))
        .run(argc, argv);
}
