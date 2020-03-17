#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <string_view>

#include "trecpp/trecpp.hpp"

using namespace trecpp;
using namespace trecpp::detail;

TEST_CASE("read_between") {
    std::string_view data =
        "<DOC>\n"
        "<DOCNO>GX000-00-0000000</DOCNO>\n"
        "<DOCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html>"
        "</DOC>";
    std::size_t pos = 0;
    auto read_between = detail::read_between(data, pos);
    auto docno = read_between(detail::DOCNO, detail::DOCNO_END);
    REQUIRE(docno);
    REQUIRE(*docno == std::string_view("GX000-00-0000000"));
    auto body = read_between(detail::DOCHDR_END, detail::DOC_END);
    REQUIRE(body);
    REQUIRE(*body == std::string_view("\n<html>"));
}

TEST_CASE("parse") {
    std::vector<std::string> data{
        "<DOC>\n"
        "<DOCNO>GX000-00-0000000</DOCNO>\n"
        "<DOCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html>"
        "</DOC>",
        "\n        \t"
        "<DOC>\n"
        "<DOCNO>GX000-00-0000001</DOCNO>\n"
        "<DCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html> 2"
        "</DOC>\n",
        "<DOC>\n"
        "<DOCNO>GX000-00-0000001</DOCNO>\n"
        "<DOCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "<<<Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html> 2"
        "</DOC>"};
    auto rec = web::parse(data[0]);
    CAPTURE(rec);
    Record *record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "GX000-00-0000000");
    REQUIRE(record->url() == "http://sgra.jpl.nasa.gov");
    REQUIRE(record->content() == "\n<html>");

    rec = web::parse(data[1]);
    CAPTURE(rec);
    REQUIRE(std::get_if<Error>(&rec) != nullptr);

    rec = web::parse(data[2]);
    CAPTURE(rec);
    record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "GX000-00-0000001");
    REQUIRE(record->url() == "http://sgra.jpl.nasa.gov");
    REQUIRE(record->content() == "\n<html> 2");
}

TEST_CASE("Read web records", "[unit]")
{
    std::istringstream is(
        "<DOC>\n"
        "<DOCNO>GX000-00-0000000</DOCNO>\n"
        "<DOCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html>"
        "</DOC>\n        \t"
        "<DOC>\n"
        "<DOCNO>GX000-00-0000001</DOCNO>\n"
        "<DCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html> 2"
        "</DOC>\n"
        "<DOC>\n"
        "<DOCNO>GX000-00-0000001</DOCNO>\n"
        "<DOCHDR>\n"
        "http://sgra.jpl.nasa.gov\n"
        "HTTP/1.1 200 OK\n"
        "<<<Date: Tue, 09 Dec 2003 21:21:33 GMT\n"
        "Server: Apache/1.3.27 (Unix)\n"
        "Last-Modified: Tue, 26 Mar 2002 19:24:25 GMT\n"
        "ETag: \"6361e-266-3ca0cae9\n"
        "\n"
        "Accept-Ranges: bytes\n"
        "Content-Length: 614\n"
        "Connection: close\n"
        "Content-Type: text/html\n"
        "</DOCHDR>\n"
        "<html> 2"
        "</DOC>");
    web::TrecParser parser(is, 10000);
    auto rec = parser.read_record();
    CAPTURE(rec);
    Record *record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "GX000-00-0000000");
    REQUIRE(record->url() == "http://sgra.jpl.nasa.gov");
    REQUIRE(record->content() == "\n<html>");
    rec = parser.read_record();
    REQUIRE(std::get_if<Error>(&rec) != nullptr);
    rec = parser.read_record();
    CAPTURE(rec);
    record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "GX000-00-0000001");
    REQUIRE(record->url() == "http://sgra.jpl.nasa.gov");
    REQUIRE(record->content() == "\n<html> 2");
    rec = parser.read_record();
    REQUIRE(std::get_if<Error>(&rec) != nullptr);
}

TEST_CASE("Consume tag", "[unit]")
{
    SECTION("Correct tag")
    {
        std::istringstream is("<DOC>");
        REQUIRE(consume(is, "<DOC>"));
        REQUIRE(is.peek() == std::ifstream::traits_type::eof());
    }
    SECTION("Incorrect at first pos")
    {
        std::istringstream is("DOC>");
        REQUIRE_FALSE(consume(is, "<DOC>"));
        std::string s;
        is >> s;
        REQUIRE(s == "DOC>");
    }
    SECTION("Incorrect at second pos")
    {
        std::istringstream is("<LOC>");
        REQUIRE_FALSE(consume(is, "<DOC>"));
        std::string s;
        is >> s;
        REQUIRE(s == "<LOC>");
    }
    SECTION("Incorrect at third pos")
    {
        std::istringstream is("<DEC>");
        REQUIRE_FALSE(consume(is, "<DOC>"));
        std::string s;
        is >> s;
        REQUIRE(s == "<DEC>");
    }
    SECTION("Incorrect at fourth pos")
    {
        std::istringstream is("<DOK>");
        REQUIRE_FALSE(consume(is, "<DOC>"));
        std::string s;
        is >> s;
        REQUIRE(s == "<DOK>");
    }
    SECTION("Skip whitespaces")
    {
        std::istringstream is(" \t\r<DOC>");
        CHECK(consume(is, "<DOC>"));
        REQUIRE(is.peek() == std::ifstream::traits_type::eof());
    }
}

TEST_CASE("Consume any tag", "[unit]")
{
    SECTION("Correct tag")
    {
        std::istringstream is("<DOC>");
        REQUIRE(*consume(is) == "DOC");
        REQUIRE(is.peek() == std::ifstream::traits_type::eof());
    }
    SECTION("Incorrect at first pos")
    {
        std::istringstream is("DOC>");
        REQUIRE_FALSE(consume(is, "<DOC>"));
        REQUIRE(consume(is) == std::nullopt);
        std::string s;
        is >> s;
        REQUIRE(s == "DOC>");
    }
    SECTION("Skip whitespaces")
    {
        std::istringstream is(" \t\r<DOC>");
        REQUIRE(*consume(is) == "DOC");
        REQUIRE(is.peek() == std::ifstream::traits_type::eof());
    }
}

TEST_CASE("Read body", "[unit]")
{
    SECTION("Before tag")
    {
        std::istringstream is("text</DOC>rest");
        REQUIRE(read_body(is, detail::DOC_END) == "text");
        std::string s;
        is >> s;
        REQUIRE(s == "rest");
    }
    SECTION("At the end")
    {
        std::istringstream is("text");
        REQUIRE(read_body(is, detail::DOC_END) == std::nullopt);
        REQUIRE(is.peek() == std::ifstream::traits_type::eof());
    }
    SECTION("With brackets")
    {
        std::istringstream is("test <a>link</a> </DOC>rest");
        REQUIRE(read_body(is, detail::DOC_END) == "test <a>link</a> ");
        std::string s;
        is >> s;
        REQUIRE(s == "rest");
    }
}

TEST_CASE("Read text record", "[unit]")
{
    std::istringstream is(
        "<DOC>\n"
        "<DOCNO> b2e89334-33f9-11e1-825f-dabc29fd7071 </DOCNO>\n"
        "<URL> https://www.washingtonpost.com/stuff </URL>\n"
        "<TITLE> title \n"
        "</TITLE>\n"
        "\n"
        "\n"
        "<HEADLINE>\n"
        " headline \n"
        "</HEADLINE>\n"
        "<TEXT> 1 < 2 and other stuff... </TEXT>\n"
        "</DOC>\n        \t"
        "<DOC>\n"
        "<DOCNO> b2e89334-33f9-11e1-825f-dabc29fd7072 </DOCNO>\n"
        "<IGNORED attr=val>ignored text</IGNORED>\n"
        "<TTL>not ignored text</TTL>\n"
        "<TEXT>"
        "<html> 2"
        "</TEXT>"
        "</DOC>\n"
        "<DOC>\n"
        "<DOCNO> b2e89334-33f9-11e1-825f-dabc29fd7073 </DOCN>\n"
        "<TEXT>\n"
        "<html> 2"
        "</TEXT>\n"
        "</DOC>\n"
        "<DOC>\n"
        "<DOCNO> b2e89334-33f9-11e1-825f-dabc29fd7071 </DOCNO>\n"
        "</DOC>");
    auto rec = text::read_record(is);
    CAPTURE(rec);
    Record *record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "b2e89334-33f9-11e1-825f-dabc29fd7071");
    REQUIRE(record->url() == "https://www.washingtonpost.com/stuff");
    REQUIRE(record->content() ==
            " title \n"
            "\n headline \n"
            " 1 < 2 and other stuff... ");
    rec = text::read_record(is);
    CAPTURE(rec);
    record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "b2e89334-33f9-11e1-825f-dabc29fd7072");
    REQUIRE(record->url() == "");
    REQUIRE(record->content() ==
            "not ignored text"
            "<html> 2");
    rec = text::read_subsequent_record(is);
    REQUIRE(std::get_if<Error>(&rec) != nullptr);
    rec = text::read_subsequent_record(is);
    record = std::get_if<Record>(&rec);
    REQUIRE(record != nullptr);
    REQUIRE(record->trecid() == "b2e89334-33f9-11e1-825f-dabc29fd7071");
    REQUIRE(record->url() == "");
    REQUIRE(record->content() == "");
}

TEST_CASE("Match result", "[unit]")
{
    Result result(Record("01", "URL", "CONTENT"));
    std::string trecid;
    // Works when mutable
    match(
        result,
        [&trecid](auto&& record){
            trecid = std::move(record.trecid()); // move out of record
            REQUIRE(record.url() == "URL");
            REQUIRE(record.content() == "CONTENT");
        },
        [](auto&& error){}
    );
    // Works when const
    match(
        result,
        [](auto const& record){
            REQUIRE(record.trecid() != "01"); // moved out so not equal
            REQUIRE(record.url() == "URL");
            REQUIRE(record.content() == "CONTENT");
        },
        [](auto&& error){}
    );
}
