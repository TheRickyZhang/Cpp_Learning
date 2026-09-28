C++ standard describes an imaginary computer instead of a x86/ARM

Differences:
- Pointers are and address and provenance (which allocation/object it came from). Is the reason why std::launder exists.
- Memory holds objects, which have type and lifetime
- Any data race is UB. Actually, lots of things are UB in general

Generally want to sanitize things in practice
