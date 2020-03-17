#pragma once

#include <algorithm>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
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
    [[nodiscard]] auto content() -> std::string && { return std::move(content_); }
    [[nodiscard]] auto content() const -> std::string const & { return content_; }
    [[nodiscard]] auto url() const -> std::string const & { return url_; }
    [[nodiscard]] auto url() -> std::string && { return std::move(url_); }
    [[nodiscard]] auto trecid() const -> std::string const & { return docno_; }
    [[nodiscard]] auto trecid() -> std::string && { return std::move(docno_); }

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

    [[nodiscard]] auto skip_ws(std::string_view const &data, std::size_t pos) -> std::size_t
    {
        auto it = std::find_if(
            &data[pos], &data[data.size()], [](unsigned char ch) { return !std::isspace(ch); });
        return pos + std::distance(&data[pos], it);
    }

    [[nodiscard]] auto skip_to_ws(std::string_view const &data, std::size_t pos) -> std::size_t
    {
        auto it = std::find_if(
            &data[pos], &data[data.size()], [](unsigned char ch) { return std::isspace(ch); });
        return pos + std::distance(&data[pos], it);
    }

    [[nodiscard]] auto read_between(std::string_view const &data, std::size_t &pos)
    {
        return [&](auto const &open, auto const &close) -> std::optional<std::string_view> {
            auto begin = data.find(open, pos);
            if (begin == std::string_view::npos) {
                return std::nullopt;
            } else {
                begin += open.size();
            }
            auto end = data.find(close, begin);
            if (end == std::string_view::npos) {
                return std::nullopt;
            }
            pos = end;
            return std::string_view(&data[begin], end - begin);
        };
    }

    [[nodiscard]] auto consumer(std::string_view &data, std::size_t &pos)
    {
        return [&](auto const &tag) {
            pos = skip_ws(data, pos);
            auto tag_len = tag.size();
            if (pos + tag_len < pos && data.substr(pos, tag_len) == tag) {
                pos += tag_len;
                return true;
            } else {
                return false;
            }
        };
    }

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
        auto tag = read_until(is, [](char ch) { return ch == '>'; });
        if (is.peek() == '>') {
            is.ignore(1);
            auto first_whitespace = std::find_if(
                tag.begin(), tag.end(), [](unsigned char ch) { return std::isspace(ch); });
            auto size = std::distance(tag.begin(), first_whitespace);
            tag.resize(size);
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

    std::string_view read_token(std::string_view const &data, std::size_t &pos)
    {
        pos = skip_ws(data, pos);
        auto first = pos;
        pos = skip_to_ws(data, pos + 1);
        return std::string_view(&data[first], pos - first);
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

template <typename R, typename Record_Handler, typename Error_Handler>
auto match(R &&result, Record_Handler &&record_handler, Error_Handler &&error_handler)
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

[[nodiscard]] auto consume_error(std::string const &tag, std::istream &is) -> Error
{
    std::string context;
    std::getline(is, context);
    auto error = Error{"Could not consume " + tag + " in context: " + context};
    is.putback('\n');
    for (auto pos = context.rbegin(); pos != context.rend(); ++pos) {
        is.putback(*pos);
    }
    return error;
}

namespace text {

    static const std::unordered_set<std::string> content_tags = {
        "TEXT", "HEADLINE", "TITLE", "HL", "HEAD", "TTL", "DD", "DATE", "LP", "LEADPARA"};

    [[nodiscard]] auto read_record(std::istream &is) -> Result
    {
        if (not detail::consume(is, detail::DOC)) {
            return consume_error(detail::DOC, is);
        }
        if (not detail::consume(is, detail::DOCNO)) {
            return consume_error(detail::DOCNO, is);
        }
        is >> std::ws;
        auto docno = detail::read_token(is);
        is >> std::ws;
        if (not detail::consume(is, detail::DOCNO_END)) {
            return consume_error(detail::DOCNO_END, is);
        }
        std::string url = "";
        std::ostringstream content;
        while (not detail::consume(is, detail::DOC_END)) {
            is >> std::ws;
            auto tag = detail::consume(is);
            if (not tag) {
                return consume_error("any tag ", is);
            }
            auto closing_tag = detail::closing_tag(*tag);
            auto body = detail::read_body(is, closing_tag);
            if (not body) {
                return consume_error(closing_tag, is);
            }
            if (tag == "URL") {
                std::copy_if(body->begin(), body->end(), std::back_inserter(url), [](char ch) {
                    return not std::isspace(ch);
                });
            } else if (content_tags.find(*tag) != content_tags.end()) {
                content << *body;
            }
        }
        return Record(std::move(docno), std::move(url), content.str());
    }

    [[nodiscard]] auto read_subsequent_record(std::istream &is) -> Result
    {
        return detail::read_subsequent_record(is, read_record);
    }

}

namespace web {

    [[nodiscard]] auto parse(std::string_view data) -> Result
    {
        std::size_t pos = 0;
        auto consume_error = [&](auto const &tag) -> Error {
            auto context_size = std::min(tag.size(), data.size() - pos);
            auto context = std::string_view(&data[pos], context_size);
            return Error{"Could not consume " + tag + " in context: " + std::string(context)};
        };
        auto read_between = detail::read_between(data, pos);

        auto docno = read_between(detail::DOCNO, detail::DOCNO_END);
        if (not docno) {
            return consume_error(detail::DOCNO);
        }

        pos = data.find(detail::DOCHDR, pos);
        if (pos == std::string_view::npos) {
            return consume_error(detail::DOCHDR);
        }
        pos += detail::DOCHDR.size();
        auto url = detail::read_token(data, pos);

        auto body = read_between(detail::DOCHDR_END, detail::DOC_END);
        if (not docno) {
            return consume_error(detail::DOCNO);
        }
        return Record(std::string(*docno), std::string(url), std::string(*body));
    }

    class TrecParser {
       public:
        TrecParser(std::istream &input, std::size_t batch_size = 10000)
            : input_(input), batch_size_(batch_size)
        {
        }
        [[nodiscard]] auto operator()() -> Result { return read_record(); }
        [[nodiscard]] auto read_record() -> Result
        {
            auto view = read_enough();
            if (not view) {
                return Error{"EOF"};
            } else {
                auto res = web::parse(*view);
                buf_.erase(buf_.begin(), buf_.begin() + view->size());
                return res;
            }
        }

       private:
        /// Reads at least enough to buffer the next record.
        /// It returns `std::nullopt` if the next record cannot be read.
        [[nodiscard]] auto read_enough() -> std::optional<std::string_view>
        {
            std::string_view view(&buf_[0], buf_.size());
            auto pos = view.find(detail::DOC_END);
            while (pos == std::string_view::npos) {
                auto old_size = buf_.size();
                buf_.resize(buf_.size() + batch_size_);
                input_.read(&buf_[old_size], batch_size_);
                if (input_.gcount() == 0) {
                    return std::nullopt;
                }
                buf_.resize(old_size + input_.gcount());
                view = std::string_view(&buf_[0], buf_.size());
                pos =
                    view.find(detail::DOC_END,
                              std::max(old_size, detail::DOC_END.size()) - detail::DOC_END.size());
            }
            return std::string_view(&buf_[0], pos + detail::DOC_END.size());
        }

        std::istream &input_;
        std::size_t batch_size_;
        std::vector<char> buf_{};
    };

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
