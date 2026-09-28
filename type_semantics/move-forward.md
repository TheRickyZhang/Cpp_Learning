# std::move
Implementation-wise, std::move is simply a cast to an rvalue reference type.

Semantically, std::move permits taking resources from the object, but does not require.

By the standard, any moved-from object should remain in a valid but unspecified state. In practice, it is usually empty.

A move constructor is allowed to internally copy, and move attempts can fall back to copying. In this sense move is a superset of copy.
A common pattern for moves, like with `std::uninitialized_move<T>`, is to use a move if 

# std::forward
Now think about issue with forwarding references: if you want to support both lvalue and moves, then you can't get both. std::forward solves the problem by preserving value type exactly
