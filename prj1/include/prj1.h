// prj1.h
// Cross-platform ZeroMQ IPC/UDP communication wrapper API
// Exposes unified API for Windows (DLL) and macOS/Linux (dylib)
//
// Copyright (c) 2025 sxtyxmm
//
// Usage Example:
//   #include "prj1.h"
//   prj1::Config cfg{...};
//   prj1::Context ctx;
//   ctx.Init(cfg);
//   ctx.SendMessage("Hello");
//   std::string msg = ctx.ReceiveMessage();
//   ctx.Close();

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
  #ifdef PRJ1_EXPORTS
    #define PRJ1_API __declspec(dllexport)
  #else
    #define PRJ1_API __declspec(dllimport)
  #endif
#else
  #define PRJ1_API __attribute__((visibility("default")))
#endif

namespace prj1 {

// Communication pattern
enum class Pattern {
    REQ_REP,
    PUB_SUB,
    PUSH_PULL
};

// Mode: server (bind) or client (connect)
enum class Mode {
    SERVER,
    CLIENT
};

// Configuration for initialization
struct Config {
    Pattern pattern;
    Mode mode;
    std::string endpoint; // e.g. "ipc:///tmp/prj1.sock" or "\\.\pipe\prj1_pipe"
    int io_threads = 1;   // ZeroMQ IO threads
    int recv_timeout_ms = 3000; // Receive timeout
    int send_timeout_ms = 3000; // Send timeout
    bool verbose = false; // Enable logging
};

// Error codes
enum class ErrorCode {
    OK = 0,
    INIT_FAILED,
    SEND_FAILED,
    RECV_FAILED,
    CLOSE_FAILED,
    INVALID_CONFIG,
    SOCKET_EXISTS,
    PERMISSION_DENIED,
    PATH_TOO_LONG,
    CONNECTION_RETRY,
    MALFORMED_MESSAGE,
    UNKNOWN
};

// Main wrapper context
class PRJ1_API Context {
public:
    Context();
    ~Context();

    // Initialize wrapper (server/client)
    ErrorCode Init(const Config& config);

    // Send message to endpoint
    ErrorCode SendMessage(const std::string& message);

    // Receive message from endpoint
    ErrorCode ReceiveMessage(std::string& out_message);

    // Close and cleanup resources
    ErrorCode Close();

    // Get last error string
    std::string GetLastError() const;

private:
    struct Impl;
    Impl* pImpl; // PIMPL for ABI stability
};

} // namespace prj1
