#include <jv/tree-algorithms.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

std::vector<std::string_view> split_tokens(std::string_view str)
{
    std::vector<std::string_view> result;
    while (true) {
        // removing spaces at beginning of str
        str.remove_prefix(std::min(str.find_first_not_of(" \r\t\n"), str.size()));
        if (str.empty())
            // no tokens left
            return result;
        // getting index of the token end
        auto token_end = std::min(str.find_first_of(" \r\t\n"), str.size());
        // adding the token to the list
        result.push_back(str.substr(0, token_end));
        // removing the token
        str.remove_prefix(token_end);
    }
}

// Using virtual dispatch (may have use std::variant instead)
struct Node {
    virtual ~Node() noexcept = default;
    virtual int getNbChildren() noexcept = 0;
    virtual double getValue(double* child_begin, double* child_end) noexcept = 0;
};
using NodePtr = std::unique_ptr<Node>;

struct Number : Node {
    double value;
    Number(double v) noexcept : value{v} {}

    // Number is a leaf node
    virtual int getNbChildren() noexcept { return 0; }

    virtual double getValue([[maybe_unused]] double* begin, [[maybe_unused]] double* end) noexcept
    {
        assert(begin == end); // ensures a number has no children
        return value;
    }
};

struct Operation : Node {
    enum Op { Add, Sub, Mult, Div, Sqrt, Pow };
    Op operation;

    // Parsing the token
    Operation(std::string_view token)
    {
        static constexpr std::string_view tokens[] = {"+", "-", "x", "/", "sqrt", "pow"};
        if (auto it = std::find(std::begin(tokens), std::end(tokens), token);
            it != std::end(tokens)) {
            operation = static_cast<Op>(it - std::begin(tokens));
        }
        else
            throw std::invalid_argument("Invalid token for Operation");
    }

    virtual int getNbChildren() noexcept
    {
        static constexpr int nb_children[] = {2, 2, 2, 2, 1, 2};
        return nb_children[static_cast<int>(operation)];
    }

    virtual double getValue(double* it, [[maybe_unused]] double* end) noexcept
    {
        assert(it + getNbNodes() == end); // ensures it has the appropriate number of children
        switch (operation) {
        case Add: return it[0] + it[1];
        case Sub: return it[0] - it[1];
        case Mult: return it[0] * it[1];
        case Div: return it[0] / it[1];
        case Sqrt: return std::sqrt(it[0]);
        case Pow: return std::pow(it[0], it[1]);
        default: return 0;
        }
    }
};

// conversion from a token  to a node
NodePtr token_to_node(std::string_view token)
{
    auto token_end = token.data() + token.size();
    char* num_end = nullptr;
    if (double value = std::strtod(token.data(), &num_end); num_end == token_end) {
        return std::make_unique<Number>(value);
    }
    return std::make_unique<Operation>(token);
}

using MathTree = std::vector<NodePtr>;

// conversion from a list of tokens to a MathTree
MathTree parse_expression(std::string_view expression)
{
    auto tokens = split_tokens(expression);
    MathTree tree(tokens.size());
    std::transform(tokens.begin(), tokens.end(), tree.begin(), token_to_node);
    return tree;
}

struct NodeTraits : jv::NodeTraits<MathTree::const_iterator, NodeTraits> {
    static std::size_t getChildrenCount(iterator it) noexcept { return (*it)->getNbChildren(); }
};

double evaluate(MathTree const& tree) noexcept
{
    auto [value, _] = NodeTraits::evaluationTraversal<double>(
        tree.begin(),
        [](auto node, double* begin, double* end) { return (*node)->getValue(begin, end); });
    return value;
}

double evaluate(std::string_view expression) noexcept
{
    return evaluate(parse_expression(expression));
}

int main()
{
    // (3 * 5) - (8 / 2) = 15 - 4 = 11
    std::string_view expression = "- x 3 5 / 8 2";
    std::cout << expression << " => " << evaluate(expression) << " (expected: 11)\n";

    // sqrt( (3^2) + (4^2) ) = sqrt(9 + 16) = 5
    expression = "sqrt + pow 3 2 pow 4 2";
    std::cout << expression << " => " << evaluate(expression) << " (expected: 5)\n";

    return 0;
}