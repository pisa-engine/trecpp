#include <memory>
#include <optional>
#include <string>

#include <CLI/CLI.hpp>

#include <trecpp/trecpp.hpp>

using trecpp::Error;
using trecpp::match;
using trecpp::Record;
using trecpp::Result;

template <class RF, class Fn>
void read(std::istream &is, RF read, Fn &&print_record)
{
    while (not is.eof()) {
        match(
            read(is),
            [&](Record const &rec) { print_record(rec); },
            [&](Error const &error) { std::clog << "Invalid version in line: " << error << '\n'; });
    }
}

auto select_print_fn(std::string const &fmt)
    -> std::function<std::function<void(Record const &)>(std::ostream &)>
{
    auto print_tsv = [](std::ostream &os) {
        return [&](Record const &rec) {
            os << rec.trecid() << '\t' << rec.url() << '\t';
            std::istringstream is(std::move(rec.content()));
            std::string line;
            while (std::getline(is, line)) {
                os << "\\u000A" << line;
            }
            os << '\n';
        };
    };
    return print_tsv;
}

int main(int argc, char **argv)
{
    bool text = false;
    std::string input;
    std::optional<std::string> output = std::nullopt;
    std::string fmt = "tsv";
    CLI::App app{
        "Parse a TREC file and output in a selected text format.\n\n"
        "Because lines delimit records, any new line characters in the content\n"
        "will be replaced by \\u000A sequence."};
    app.add_option("input", input, "Input file(s); use - to read from stdin")->required();
    app.add_option("output", output, "Output file; if missing, write to stdout");
    app.add_option("-f,--format", fmt, "Output file format", true)->check(CLI::IsMember({"tsv"}));
    app.add_flag("--text", text, "Use trectext format rather than trecweb (default)");
    CLI11_PARSE(app, argc, argv);

    auto print = select_print_fn(fmt);

    std::istream *is = &std::cin;
    std::unique_ptr<std::ifstream> file_is = nullptr;
    if (input != "-") {
        file_is = std::make_unique<std::ifstream>(input);
        is = file_is.get();
    }

    std::ostream *os = &std::cout;
    std::unique_ptr<std::ofstream> file_os = nullptr;
    if (output) {
        file_os = std::make_unique<std::ofstream>(*output);
        os = file_os.get();
    }

    if (text) {
        read(*is, trecpp::text::read_subsequent_record, print(*os));
    } else {
        auto print_record = print(*os);
        trecpp::web::TrecParser parser(*is);
        while (not is->eof()) {
            match(
                parser.read_record(),
                [&](Record const &rec) { print_record(rec); },
                [&](Error const &error) { std::clog << "Invalid record: " << error << '\n'; });
        }
    }
    return 0;
}
