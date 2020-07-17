# Viaduct Runtime

This is the runtime library for the [Viaduct](https://github.com/apl-cornell/viaduct) compiler.

## Dependencies

- Boost.System
- pthreads

## Build instructions

This project uses CMake for its build system. To build, run these commands in your terminal
with the root of the repository as the current directory:

```
mkdir build && cd build
cmake ..
make
```

You should see some binaries in the `build/bin` directory. The examples take in the
host ID as a command-line argument. To run the `ping_pong` test, for example, you can run
`./ping_pong 0` on one terminal and `./ping_pong 1` on another.
