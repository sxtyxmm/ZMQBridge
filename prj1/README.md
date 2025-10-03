# prj1: Cross-Platform ZeroMQ IPC/UDP Communication Wrapper

## Overview
prj1 is a C++ library providing a unified, cross-platform API for ZeroMQ-based IPC and UDP communication. It statically links a vendored ZeroMQ (libzmq-4.3.5) and exposes a clean API via DLL (Windows) or dylib (macOS/Linux), requiring no separate ZeroMQ installation for consumers.

## Features
- Statically linked ZeroMQ (libzmq-4.3.5)
- Unified API for REQ/REP, PUB/SUB, PUSH/PULL patterns
- IPC via Unix Domain Sockets (macOS/Linux) and Windows Named Pipes
- Simple API: Init, SendMessage, ReceiveMessage, Close
- Handles edge cases: permissions, retries, multi-threading, large/malformed messages
- CMake-based cross-platform build
- Example server/client usage

## Directory Structure
```
prj1/
 ├─ include/         # Public headers
 ├─ src/             # Wrapper implementation
 ├─ lib/zeromq/      # Vendored ZeroMQ source
 ├─ build/           # Output DLL/dylib
 └─ CMakeLists.txt
```

## Usage
See `include/prj1.h` for API documentation and `src/examples/` for server/client usage.

## Build
Use CMake to build DLL/dylib. No external ZeroMQ installation required.

---
