Conceptually, std::function abstraction over some callable.

It is very similar to how std::string is abstracting over a const char*.

Inefficient because can't be inlined - so has downstream affects of branch miss prediction / instructing cache miss
