trecpp
======

[![Build Status](https://travis-ci.com/pisa-engine/trecpp.svg?branch=master)](https://travis-ci.org/pisa-engine/trecpp)

A single-header C++ parser for the TREC Web format.

## Usage

### `namespace trecpp`

```cpp
[[nodiscard]] auto read_record(std::istream&) -> Result;
```
Reads a record from the current position in the stream.

```cpp
[[nodiscard]] auto read_subsequent_record(std::istream&) -> Result;
```
Same as `read_record` but will skip any junk before the first
valid beginning of a record (`<DOC>` tag).

### Pattern Matching

`Result` is an alias for `std::variant<Record, Error>`.
For convenience, `match` function is provided to enable basic pattern matching.
The following example shows how to iterate over a TREC collection
and retrieve the total size of all valid records.

```cpp
#include <iostream>
#include <trecpp/trecpp.hpp>

using trecpp::match;
using trecpp::Result;
using trecpp::Error;
using trecpp::Record;

std::size_t total_content_size(std::ifstream& is)
{
    std::size_t total{0u};
    while (not is.eof())
    {
        total += match(
            trecpp::read_subsequent_record(in), // will skip to first valid beginning
            [](Record const &rec) -> std::size_t {
                return rec.content_length();
            },
            [](Error const &error) -> std::size_t {
                std::clog << "Error: " << error << '\n';
                return 0u;
            });
    }
    return total;
}
```

### Classic Approach

Alternatively, you can use `holds_record` and `std::get<T>(std::variant)`
functions to access returned data:

```cpp
#include <iostream>
#include <trecpp/trecpp.hpp>

using trecpp::holds_record;

std::size_t total_content_size(std::ifstream& is)
{
    std::size_t total{0u};
    while (not is.eof())
    {
        auto record = trecpp::read_subsequent_record(in);
        if (holds_record(record)) {
            total += std::get<Record>(record).content_length();
        }
        else {
            // You actually don't need to cast in order to print
            std::clog << "Warn: " << record << '\n';
        }
    }
    return total;
}
```
