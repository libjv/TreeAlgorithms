# Reference of <jv/tree-algorithms.hpp>

This is the reference page for TreeAlgorithms. To see the summary and examples, [go here](index.md).

## NodeTraits creation
You must provide the access to the number of children of each node.
This is done by creating you own NodeTraits deriving from `jv::NodeTraits`.
The method name and signature must match exactly the one provided below.
```cpp
struct MyNodeTraits : jv::NodeTraits<MyIterator, MyNodeTraits> {
    static std::size_t getChildrenCount(iterator it) noexcept {
        ... the code ...
    }
};
```

`jv::NodeTraits` has two template parameters:

+ `InputIterator`: the appropriate iterator to the sequence, which must satisfy the `InputIterator` requirement.
+ `Crtp`: the NodeTraits you are creating, this is needed for dispatching function calls

After this, the NodeTraits you have created has the following methods:

## recursiveTraversal(node, func)

### Interface
```cpp
template <typename Func>
static iterator recursiveTraversal(iterator node, Func&& func);
```

### Parameters
+ **node** (_iterator_): The node for which we will traverse its children
+ **func** (_Func_): A function that matches the signature `(iterator, Func&)->iterator`
+ returns (_iterator_): Pointer to the next sibling of **node**

### Description
This function is intended to be called inside the `func` you provide, this is how the recursion is made.
Any function based on `recursiveTraversal` has the following structure:

```cpp
void myFunction(iterator root) {
    ... initialisation ...
    recursiveTraversal(root, [] (iterator node, auto& self) {
        ... before children traversal ...
        iterator next_node = recursiveTraversal(node, self);
        ... after children traversal ...
        return next_node;
    });
}
```

### Notes
This is a low-level function, but it provides the most customisation possible.
`ancestorsTraversal` and `evaluationTraversal` are implemented using `recursiveTraversal`.
You can check the implementation of these functions to have examples on how to use `recursiveTraversal`.

## ancestorsTraversal(node, func, alloc = {})

### Interface
```cpp
template <typename Allocator = std::allocator<iterator>, typename Func>
static iterator ancestorsTraversal(iterator root, Func&& func, Allocator alloc = {})
```

### Parameters
+ **root** (_iterator_): The node which is the root of the tree
+ **func** (_Func_): A function that can be called with `(iterator* begin, iterator* end)`
+ **alloc** (_Allocator_): An allocator used internally for a `vector<iterator, Allocator>`
+ returns (_iterator_): Pointer to the next sibling of **node**

### Description
This function calls `func` for every node, and it provides a range to all the ancestors of the current node.
For instance, with the following tree:
```
A
|-> B
|   |-> C
|   |-> D
|-> E
|-> F
    |-> G
    |-> H
```

`func` will be called 8 times, with the following ranges:
```cpp
Call #1: { A }
Call #2: { A, B }
Call #3: { A, B, C }
Call #4: { A, B, D }
Call #5: { A, E }
Call #6: { A, F }
Call #7: { A, F, G }
Call #8: { A, F, H }
```

## evaluationTraversal<Value>(node, func, alloc = {})

### Interface
```cpp
template <typename Value, typename Allocator = std::allocator<Value>, typename Func>
static std::pair<Value, iterator>
    evaluationTraversal(iterator root, Func&& func, Allocator alloc = {})
```

### Parameters
+ **root** (_iterator_): The node which is the root of the tree
+ **func** (_Func_): A function that can be called with `(iterator node, Value* begin, Value* end)`
+ **alloc** (_Allocator_): An allocator used internally for a `vector<Value, Allocator>`
+ returns (_`pair<Value, iterator>`_): Pointer to the next sibling of **node**

### Description
This function calls `func` for every node, starting by children.
`func` produces intermediary values of type `Value` that are passed to the parent.
For instance, with the following tree:
```
A
|-> B
|   |-> C
|   |-> D
|-> E
|-> F
    |-> G
    |-> H
```

`func` will be called 8 times, with the following ranges:
```cpp
// indentation is only for visualization purposes
Call #1:     value_C = func(C, {})
Call #2:     value_D = func(D, {})
Call #3:   value_B = func(B, { value_C, value_D })
Call #4:   value_E = func(E, {})
Call #5:     value_G = func(G, {})
Call #6:     value_H = func(H, {})
Call #7:   value_F = func(F, { value_G, value_H })
Call #8: value_A = func(A, { value_B, value_E, value_F })
```