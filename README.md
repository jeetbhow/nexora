# mininode

An HTTP framework for C++23 in the style of Express.js, built on raw POSIX
sockets and epoll with no dependencies beyond the standard library. It is a
work in progress; see the roadmap below.

## Building

You need a C++ compiler with `std::expected` (GCC 12+ or Clang 19+), Meson, and
Ninja. The Makefile wraps the usual steps and builds with clang-20:

```sh
make          # configure and build
make test     # build and run the tests
make run ARGS="--help"  # build and show CLI usage
```

Or run Meson directly:

```sh
meson setup build
meson compile -C build
meson test -C build
./build/app/cli --help
```

## Layout

- `include/http/` - public headers of the `http` library
- `src/` - the library's sources, built as the static library `libhttp`
- `app/` - a CLI for request parsing and route matching
- `tests/` - GoogleTest unit tests

## Roadmap

- [ ] Request parsing — request line, headers, body, and query parameters complete;
  path percent-decoding remains
- [ ] Router — static, parameter, and terminal wildcard matching complete;
  public match results and wildcard captures remain
- [ ] Response
- [ ] Route registration
- [ ] Middleware
- [ ] Server
- [ ] Polish

See [docs/tasks.md](docs/tasks.md) for the detailed checklist and
[docs/design.md](docs/design.md) for current behavior and the planned framework.
