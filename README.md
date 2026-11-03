# Nexora

An HTTP framework for C++23 in the style of Express.js, built on raw POSIX
sockets and epoll with minimal dependencies. It is a work in progress; see the
roadmap below.

## Building

You need a C++ compiler with `std::expected` (GCC 12+ or Clang 19+), Meson, and
Ninja. The Makefile wraps the usual steps and builds with clang-20:

```sh
make          # configure and build
make test     # build and run the tests
make run      # build and run main.cpp
```

Or run Meson directly:

```sh
meson setup build
meson compile -C build
meson test -C build
```

## Layout

- `include/nexora/` - public headers of the `nexora` library
- `src/` - the library's sources, built as the static library `libnexora`
- `tests/` - GoogleTest unit tests

## Roadmap

- [ ] Request parsing — request line, headers, body, and query parameters complete
- [ ] Router — static, parameter, and terminal wildcard matching complete
- [x] Response
- [x] Route registration
- [ ] Middleware
- [ ] Server — socket binding and listening complete
- [ ] Polish
