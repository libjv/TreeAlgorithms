#include "catch.hpp"
#include <jv/tree-algorithms.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using std::string;
using std::string_view;

struct File {
    string_view name;
    int size;
};

struct Directory {
    string_view name;
    int nb_children;
};

using Entry = std::variant<File, Directory>;
using Entries = std::vector<Entry>;

// clang-format off
Entries entries {
    Directory{"TreeAlgorithms", 3},
        File{"README.md", 100},
        Directory{"src", 2},
            Directory{"jv", 1},
                File{"tree-algorithms.hpp", 800},
            File{"main.cpp", 400},
        File{"LICENSE", 200},
};
// clang-format on

struct EntryTraits : jv::NodeTraits<Entries::iterator, EntryTraits> {
    static auto getChildrenCount(iterator it) noexcept -> std::size_t
    {
        if (Directory* dir = std::get_if<Directory>(&*it))
            return dir->nb_children;
        else
            return 0;
    }
};

TEST_CASE("getNextSibling")
{
    std::vector<string_view> result;

    auto it = entries.begin() + 1, // pointing to README.md
        end = entries.end();
    for (; it != end; it = EntryTraits::getNextSibling(it))
        std::visit([&](auto& value) { result.push_back(value.name); }, *it);

    std::vector<string_view> expected{"README.md", "src", "LICENSE"};
    CHECK(result == expected);

    // TreeAlgorithms has no siblings
    CHECK(EntryTraits::getNextSibling(entries.begin()) == entries.end());
}

TEST_CASE("ancestorsTraversal")
{
    std::vector<string> result;

    auto next = EntryTraits::ancestorsTraversal(entries.begin(), [&](auto it, auto end) {
        std::string str;
        for (; it != end; ++it)
            if (auto dir = std::get_if<Directory>(&**it))
                str += std::string(dir->name) + "/";
            else
                str += std::string(std::get<File>(**it).name);
        result.emplace_back(std::move(str));
    });

    CHECK(next == entries.end());
    std::vector<string> expected{"TreeAlgorithms/",
                                 "TreeAlgorithms/README.md",
                                 "TreeAlgorithms/src/",
                                 "TreeAlgorithms/src/jv/",
                                 "TreeAlgorithms/src/jv/tree-algorithms.hpp",
                                 "TreeAlgorithms/src/main.cpp",
                                 "TreeAlgorithms/LICENSE"};
    CHECK(result == expected);
}

TEST_CASE("evaluationTraversal")
{
    auto [value, next] =
        EntryTraits::evaluationTraversal<int>(entries.begin(), [](auto node, auto it, auto end) {
            if (auto file = std::get_if<File>(&*node)) {
                REQUIRE(it == end);
                return file->size;
            }
            else {
                return std::accumulate(it, end, 0);
            }
        });

    CHECK(next == entries.end());
    CHECK(value == 1500);
}