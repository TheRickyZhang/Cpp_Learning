Walk through compilation -> running main():

Compilation:
- Preprocessing (removed with modules)
- Lexing -> tokens
- Parsing -> AST
- Semantic analysis: type checking, name lookup
- IR generation: lower AST to IR
- Optimization
- Code Generation: machine code, register allocation, instruction scheduling
- Assembly: object file, links includes libraries

Then, ./a.out -> shell forks, execs ./a.out with argv, envp
- Kernel reads file header (ELF), mmaps the segments into the new address space
- In dynamically linked executable, loads interpreter
- linked runs in userspace: loads shared libraries, and does relocations: patches addresses based on where the definition lives
- Jumps to entry point, runs global constructors, then runs main

Note HFT prefers static (copied inline), since it avoids an extra load that the PLT/GOT need to lazily bind

Everything else prefers dynamic, since it allows for memory sharing and shorter compile times on updates.


