#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <unordered_set>
#include <variant>

namespace trecpp {

struct Error {
    std::string msg;
};
class Record;
using Result = std::variant<Record, Error>;

class Record {
   private:
    std::string docno_;
    std::string url_;
    std::string content_;

   public:
    Record(std::string docno, std::string url, std::string content)
        : docno_(std::move(docno)), url_(std::move(url)), content_(std::move(content))
    {}
    [[nodiscard]] auto content_length() const -> std::size_t { return content_.size(); }
    [[nodiscard]] auto content() -> std::string & { return content_; }
    [[nodiscard]] auto content() const -> std::string const & { return content_; }
    [[nodiscard]] auto url() const -> std::string const & { return url_; }
    [[nodiscard]] auto trecid() const -> std::string const & { return docno_; }

    friend std::ostream &operator<<(std::ostream &os, Record const &record);
};

namespace detail {

    static std::string const DOC = "<DOC>";
    static std::string const DOC_END = "</DOC>";
    static std::string const DOCNO = "<DOCNO>";
    static std::string const DOCNO_END = "</DOCNO>";
    static std::string const DOCHDR = "<DOCHDR>";
    static std::string const DOCHDR_END = "</DOCHDR>";
    static std::string const URL = "<URL>";
    static std::string const URL_END = "</URL>";

    bool consume(std::istream &is, std::string const &token)
    {
        is >> std::ws;
        for (auto pos = token.begin(); pos != token.end(); ++pos) {
            if (is.get() != *pos) {
                is.unget();
                for (auto rpos = std::reverse_iterator(pos); rpos != token.rend(); ++rpos) {
                    is.putback(*rpos);
                }
                return false;
            }
        }
        return true;
    }

    template <typename Pred>
    std::ostream& read_until(std::istream &is, Pred pred, std::ostream& os)
    {
        while (not is.eof()) {
            if (is.peek() == std::istream::traits_type::eof() or pred(is.peek())) {
                break;
            }
            os.put(is.get());
        }
        return os;
    }

    template <typename Pred>
    std::string read_until(std::istream &is, Pred pred)
    {
        std::ostringstream os;
        read_until(is, pred, os);
        return os.str();
    }

    std::optional<std::string> consume(std::istream &is)
    {
        is >> std::ws;
        if (is.peek() == std::istream::traits_type::eof() or is.peek() != '<') {
            return std::nullopt;
        }
        is.ignore(1);
        auto tag = read_until(is, [](char ch) { return ch == '>' or std::isspace(ch); });
        if (is.peek() == '>') {
            is.ignore(1);
            return std::make_optional(tag);
        }
        return std::nullopt;
    }

    std::optional<std::string> read_body(std::istream &is, std::string const &closing_tag)
    {
        std::ostringstream os;
        while (read_until(is, [](auto ch) { return ch == '<'; }, os)) {
            if (is.peek() == std::istream::traits_type::eof()) {
                break;
            }
            if (consume(is, closing_tag)) {
                return std::make_optional(os.str());
            }
            os.put(is.get());
        }
        return std::nullopt;
    }

    std::string read_token(std::istream &is)
    {
        return read_until(is, [](char c) { return c == '<' || std::isspace(c); });
    }

    template <class ReadRecord>
    [[nodiscard]] auto read_subsequent_record(std::istream &is, ReadRecord read_record) -> Result
    {
        is.ignore(std::numeric_limits<std::streamsize>::max(), '<');
        if (is.eof()) {
            return Error{"EOF"};
        }
        is.putback('<');
        while (not is.eof() and not detail::consume(is, detail::DOC)) {
            is.ignore(1);
            is.ignore(std::numeric_limits<std::streamsize>::max(), '<');
            is.putback('<');
        }
        if (not is.eof()) {
            for (auto pos = detail::DOC.rbegin(); pos != detail::DOC.rend(); ++pos) {
                is.putback(*pos);
            }
            return read_record(is);
        }
        return Error{"EOF"};
    }

    [[nodiscard]] auto closing_tag(std::string const &tag) -> std::string
    {
        std::string ct;
        ct.reserve(tag.size() + 3);
        ct.push_back('<');
        ct.push_back('/');
        std::copy(tag.begin(), tag.end(), std::back_inserter(ct));
        ct.push_back('>');
        return ct;
    }

} // namespace detail

template <typename Record_Handler, typename Error_Handler>
auto match(Result const &result, Record_Handler &&record_handler, Error_Handler &&error_handler)
{
    if (auto *record = std::get_if<Record>(&result); record != nullptr) {
        if constexpr (std::is_same_v<decltype(record_handler(*record)), void>) {
            record_handler(*record);
        } else {
            return record_handler(*record);
        }
    } else {
        auto *error = std::get_if<Error>(&result);
        if constexpr (std::is_same_v<decltype(error_handler(*error)), void>) {
            error_handler(*error);
        } else {
            return error_handler(*error);
        }
    }
}

constexpr bool holds_record(Result const &result) { return std::holds_alternative<Record>(result); }

[[nodiscard]] auto consume_error(std::string const& tag) -> Error
{
    return Error{"Could not consume " + tag};
}

namespace text {

    static const std::unordered_set<std::string> content_tags = {
        "TEXT", "HEADLINE", "TITLE", "HL", "HEAD", "TTL", "DD", "DATE", "LP", "LEADPARA"};

    [[nodiscard]] auto read_record(std::istream &is) -> Result
    {
        if (not detail::consume(is, detail::DOC)) {
            return consume_error(detail::DOC);
        }
        if (not detail::consume(is, detail::DOCNO)) {
            return consume_error(detail::DOCNO);
        }
        is >> std::ws;
        auto docno = detail::read_token(is);
        is >> std::ws;
        if (not detail::consume(is, detail::DOCNO_END)) {
            return consume_error(detail::DOCNO_END);
        }
        std::string url = "";
        std::ostringstream content;
        while (not detail::consume(is, detail::DOC_END)) {
            is >> std::ws;
            auto tag = detail::consume(is);
            if (not tag) {
                return consume_error("any tag");
            }
            auto closing_tag = detail::closing_tag(*tag);
            auto body = detail::read_body(is, closing_tag);
            if (not body) {
                return consume_error(closing_tag);
            }
            if (tag == "URL") {
                std::copy_if(body->begin(), body->end(), std::back_inserter(url), [](char ch) {
                    return not std::isspace(ch);
                });
            } else if (content_tags.find(*tag) != content_tags.end()) {
                content << *body;
            }
        }
        return Record(std::move(docno), std::move(url), std::move(content.str()));
    }

    [[nodiscard]] auto read_subsequent_record(std::istream &is) -> Result
    {
        return detail::read_subsequent_record(is, read_record);
    }

}

namespace web {

    [[nodiscard]] auto read_record(std::istream &is) -> Result
    {
        if (not detail::consume(is, detail::DOC)) {
            return consume_error(detail::DOC);
        }
        if (not detail::consume(is, detail::DOCNO)) {
            return consume_error(detail::DOCNO);
        }
        is >> std::ws;
        auto docno = detail::read_token(is);
        if (not detail::consume(is, detail::DOCNO_END)) {
            return consume_error(detail::DOCNO_END);
        }
        if (not detail::consume(is, detail::DOCHDR)) {
            return consume_error(detail::DOCHDR);
        }
        is >> std::ws;
        auto url = detail::read_token(is);
        is.ignore(std::numeric_limits<std::streamsize>::max(), '<');
        if (not is) {
            return consume_error(detail::DOCHDR_END);
        }
        is.putback('<');
        while (not is.eof() and not detail::consume(is, detail::DOCHDR_END)) {
            is.ignore(1);
            is.ignore(std::numeric_limits<std::streamsize>::max(), '<');
            is.putback('<');
        }
        is >> std::ws;
        auto content = detail::read_body(is, detail::DOC_END);
        if (not content) {
            return consume_error(detail::DOC_END);
        }
        return Record(std::move(docno), std::move(url), std::move(*content));
    }

    [[nodiscard]] auto read_subsequent_record(std::istream &is) -> Result
    {
        return detail::read_subsequent_record(is, read_record);
    }

} // namespace web

std::ostream &operator<<(std::ostream &os, Record const &record)
{
    os << "Record {\n";
    os << "\t" << record.docno_ << "\n";
    os << "\t" << record.url_ << "\n";
    return os << "}";
}

std::ostream &operator<<(std::ostream &os, Result const &result)
{
    match(result,
          [&](Record const &record) { os << record; },
          [&](Error const &error) { os << error.msg; });
    return os;
}

} // namespace trecpp
