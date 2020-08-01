//          Copyright Julien Vernay 2020.
// Distributed under the Boost Software License, Version 1.0.
//        (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef JVERNAY_UTILS_TREE_ALGORITHMS_HPP
#define JVERNAY_UTILS_TREE_ALGORITHMS_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

namespace jv {

/// Provides functions to access nodes properties.
/// Uses the CRTP pattern.
/// Example of customization:
///     struct MyNodeTraits : NodeTraits<It, MyNodeTraits> {
///         static std::size_t getChildrenCount(iterator node) noexcept {
///             return node->get_nb_children();
///         }
///     }:
template <typename InputIterator, typename Crtp>
struct NodeTraits {
    using iterator = InputIterator;

    /// Returns the number of children this node has.
    static constexpr std::size_t getChildrenCount(iterator node) noexcept = delete;

    /// Iterates to the next sibling of the node.
    static constexpr iterator getNextSibling(iterator node) noexcept
    {
        std::size_t remaining_nodes = 1;
        do {
            remaining_nodes = remaining_nodes + Crtp::getChildrenCount(node) - 1;
            ++node;
        } while (remaining_nodes > 0);
        return node;
    }

    /// Base algorithm to recursively traverse the tree.
    /// Example:
    ///     void my_function(iterator root) {
    ///         /* init */
    ///         recursive_traversal(root, [&](iterator node, auto& self) {
    ///             /* code to evaluate before node's children
    ///             auto next_node = recursive_traversal(node, self);
    ///             /* code to evaluate after node's children
    ///             return next_node;
    ///         });
    ///     }
    /// Returns an iterator to the end of the tree.
    template <typename Func>
    static iterator recursiveTraversal(iterator node, Func&& func)
    {

        std::size_t nb_children = Crtp::getChildrenCount(node);
        ++node;
        for (; nb_children != 0; --nb_children) {
            node = func(node, func);
        }
        return node;
    }

    /// Stores parent nodes (ancestors) and evaluates the given function.
    /// The function must be invocable with (iterator* begin, iterator* end)
    template <typename Allocator = std::allocator<iterator>, typename Func>
    static iterator ancestorsTraversal(iterator root, Func&& func, Allocator alloc = {})
    {
        static_assert(std::is_invocable_v<Func, iterator*, iterator*>,
                      "Func must be invocable with (iterator*, iterator*)");

        std::vector<iterator, Allocator> ancestors(alloc);
        ancestors.push_back(root);
        func(ancestors.data(), ancestors.data() + ancestors.size());

        return recursiveTraversal(root, [&](iterator node, auto& self) {
            ancestors.push_back(node);
            func(ancestors.data(), ancestors.data() + ancestors.size());
            auto next = recursiveTraversal(node, self);
            ancestors.pop_back();
            return next;
        });
    }

    /// For each node, we evaluate the given function by passing the result of evaluation of its children.
    /// Func must match the signature (iterator node, Value* begin, Value* end) -> Value
    template <typename Value, typename Allocator = std::allocator<Value>, typename Func>
    static std::pair<Value, iterator>
    evaluationTraversal(iterator root, Func&& func, Allocator alloc = {})
    {
        static_assert(std::is_invocable_r_v<Value, Func, iterator, Value*, Value*>,
                      "Func must match the signature (iterator, Value*, Value*) -> Value");

        std::vector<Value, Allocator> values(alloc);

        auto next = recursiveTraversal(root, [&](iterator node, auto& self) {
            std::size_t begin_index = values.size();
            auto next = recursiveTraversal(node, self);
            auto ret = func(node, values.data() + begin_index, values.data() + values.size());
            values.resize(begin_index);
            values.emplace_back(ret);
            return next;
        });

        return {func(root, values.data(), values.data() + values.size()), next};
    }
};

} // namespace jv

#endif