# TeX in C++

This is an attempt to rewrite TeX and related tools in C++. Our goal is a fully bit-compliant re-implementation of TeX and its ecosystem.

![Build Status](https://github.com)
![C++ Version](https://shields.io)
![Tests](https://shields.io)

## Status
We started with tangle to gain experience. This version of tangle is fully bit-compliant with 
[tangle.p](https://github.com/classic-tools/TeX-FPC/blob/main/tangle.p) from [TeX-FPC](https://github.com/classic-tools/TeX-FPC).
It runs on Windows, MacOS and Ubuntu with all original web-files for which TeX-FPC provides change-files. The result is exactly the same as what tangle.p produces.

Even though, it is implemented in C++, this version of tangle is not very idiomatic. This is because we wanted to have a fully compliant version first. 
It was written by hand according to the tangle documentation generated from tangle.web, and manually converted, staying close to the original.

In some respects, we deviated from the original: we couldn't bring ourselves to use gotos, and sometimes renamed variables to better understand what's going on.

The next step is an extensive refactoring until it looks like a project that had started as C++. Note, that tangle does not produce C++ code and will never do.
It will not be used to generate TeX as C++ (there are other projects for this). Its only purpose is to be a playground before we tackle TeX.


## CI/CD Validation Suite
The project utilizes automated pipelines triggered on every `push` and `pull_request` to enforce compile-time compliance and algorithmic integrity:
* **Matrix Architecture**: Validates code simultaneously across `Ubuntu (GCC 14)`, `Windows (MSVC)`, and `macOS (Clang)`.
* **Testing Engine**: Automatically spins up native runners to process unit tests via `CMake` and `GoogleTest` with zero manual intervention.
