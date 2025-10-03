# ZMQBridge

Cross-platform C++ wrapper for ZeroMQ providing reusable IPC/UDP messaging utilities via a clean DLL/.dylib API.

## Overview

ZMQBridge (prj1) is a production-ready C++ library that wraps ZeroMQ functionality to provide simple, cross-platform inter-process communication (IPC). It abstracts platform-specific details and provides a clean API for developers to integrate messaging capabilities into their applications without installing ZeroMQ separately.

## Features

- **Cross-Platform**: Works on Windows, macOS, and Linux
- **Multiple Patterns**: Supports REQ/REP, PUB/SUB, and PUSH/PULL messaging patterns
- **Static Linking**: ZeroMQ is statically linked, no separate installation required
- **Thread-Safe**: All operations are thread-safe for concurrent use
- **Clean API**: Simple, well-documented interface using PIMPL pattern for ABI stability
- **Comprehensive Error Handling**: Detailed error codes and messages
- **Edge Case Handling**: Manages socket files, permissions, large messages, timeouts, etc.

## Architecture

### Communication Patterns

1. **REQ/REP (Request-Reply)**: Synchronous request-response pattern
   - Server receives requests and sends replies
   - Client sends requests and waits for replies
   - Must alternate: send → receive → send → receive

2. **PUB/SUB (Publish-Subscribe)**: One-to-many broadcast
   - Server (publisher) broadcasts messages
   - Clients (subscribers) receive all published messages
   - Subscribers can filter by topic

3. **PUSH/PULL (Pipeline)**: Load distribution pattern
   - Server pushes tasks to workers
   - Clients (workers) pull and process tasks
   - Load balanced across workers

### Platform-Specific IPC

- **Unix/Linux/macOS**: Unix Domain Sockets (`ipc:///tmp/prj1.sock`)
- **Windows**: Named Pipes (`ipc://\\.\pipe\prj1_pipe`)

## Building

### Prerequisites

- CMake 3.12 or later
- C++14 compatible compiler (GCC, Clang, MSVC)
- Git (for cloning)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/sxtyxmm/ZMQBridge.git
cd ZMQBridge

# Download and setup ZeroMQ (first time only)
./setup_zeromq.sh

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run tests
./prj1_test
```

### Build Options

```bash
# Disable examples
cmake -DBUILD_EXAMPLES=OFF ..

# Disable tests
cmake -DBUILD_TESTS=OFF ..
```

## Usage

### Basic Example (REQ/REP)

**Server:**

```cpp
#include "prj1.h"
#include <iostream>

using namespace prj1;

int main() {
    ZMQWrapper server;
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::SERVER;
    config.timeout_ms = 5000;
    config.enable_logging = true;
    
    if (server.Init(config) != ErrorCode::SUCCESS) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    std::string request;
    if (server.ReceiveMessage(request) == ErrorCode::SUCCESS) {
        std::cout << "Received: " << request << std::endl;
        server.SendMessage("Echo: " + request);
    }
    
    server.Close();
    return 0;
}
```

**Client:**

```cpp
#include "prj1.h"
#include <iostream>

using namespace prj1;

int main() {
    ZMQWrapper client;
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 5000;
    
    if (client.Init(config) != ErrorCode::SUCCESS) {
        std::cerr << "Failed to initialize client" << std::endl;
        return 1;
    }
    
    client.SendMessage("Hello Server");
    
    std::string reply;
    if (client.ReceiveMessage(reply) == ErrorCode::SUCCESS) {
        std::cout << "Received: " << reply << std::endl;
    }
    
    client.Close();
    return 0;
}
```

### PUB/SUB Example

**Publisher (Server):**

```cpp
Config config;
config.pattern = Pattern::PUB_SUB;
config.mode = Mode::SERVER;

ZMQWrapper publisher;
publisher.Init(config);

for (int i = 0; i < 10; i++) {
    publisher.SendMessage("Message #" + std::to_string(i));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

publisher.Close();
```

**Subscriber (Client):**

```cpp
Config config;
config.pattern = Pattern::PUB_SUB;
config.mode = Mode::CLIENT;

ZMQWrapper subscriber;
subscriber.Init(config);
subscriber.Subscribe("");  // Subscribe to all messages

for (int i = 0; i < 10; i++) {
    std::string message;
    if (subscriber.ReceiveMessage(message) == ErrorCode::SUCCESS) {
        std::cout << "Received: " << message << std::endl;
    }
}

subscriber.Close();
```

### PUSH/PULL Example

**Pusher (Server):**

```cpp
Config config;
config.pattern = Pattern::PUSH_PULL;
config.mode = Mode::SERVER;

ZMQWrapper pusher;
pusher.Init(config);

for (int i = 0; i < 100; i++) {
    pusher.SendMessage("Task #" + std::to_string(i));
}

pusher.Close();
```

**Worker (Client):**

```cpp
Config config;
config.pattern = Pattern::PUSH_PULL;
config.mode = Mode::CLIENT;

ZMQWrapper worker;
worker.Init(config);

while (true) {
    std::string task;
    if (worker.ReceiveMessage(task) == ErrorCode::SUCCESS) {
        // Process task
        std::cout << "Processing: " << task << std::endl;
    }
}

worker.Close();
```

## API Reference

### Configuration

```cpp
struct Config {
    Pattern pattern;        // Communication pattern (REQ_REP, PUB_SUB, PUSH_PULL)
    Mode mode;              // Server or client mode
    std::string endpoint;   // Custom endpoint (optional, empty for default)
    int timeout_ms;         // Timeout for receive operations (default: 5000ms)
    bool enable_logging;    // Enable internal logging (default: false)
};
```

### Main API

```cpp
class ZMQWrapper {
public:
    ErrorCode Init(const Config& config);
    ErrorCode SendMessage(const std::string& message);
    ErrorCode ReceiveMessage(std::string& message);
    ErrorCode Subscribe(const std::string& topic = "");
    ErrorCode Close();
    bool IsInitialized() const;
    static std::string GetErrorMessage(ErrorCode code);
};
```

### Error Codes

- `SUCCESS`: Operation completed successfully
- `ERROR_NOT_INITIALIZED`: Wrapper not initialized
- `ERROR_ALREADY_INITIALIZED`: Already initialized
- `ERROR_INVALID_CONFIG`: Invalid configuration
- `ERROR_SOCKET_CREATE_FAILED`: Socket creation failed
- `ERROR_SOCKET_BIND_FAILED`: Socket bind failed
- `ERROR_SOCKET_CONNECT_FAILED`: Socket connection failed
- `ERROR_SEND_FAILED`: Message send failed
- `ERROR_RECEIVE_FAILED`: Message receive failed
- `ERROR_TIMEOUT`: Operation timed out
- `ERROR_MESSAGE_TOO_LARGE`: Message exceeds size limit (10MB)
- `ERROR_INVALID_PATTERN`: Invalid communication pattern
- `ERROR_PERMISSION_DENIED`: Permission denied
- `ERROR_PATH_TOO_LONG`: Endpoint path too long

## Edge Cases & Error Handling

The library handles various edge cases:

- **Socket File Management**: Automatically creates and removes Unix domain socket files
- **Permission Errors**: Detects and reports permission issues
- **Path Length**: Validates endpoint path length
- **Large Messages**: Supports messages up to 10MB
- **Empty Messages**: Properly handles zero-length messages
- **Timeouts**: Configurable receive timeouts
- **Connection Retries**: Automatic retry logic for client connections
- **Thread Safety**: Mutex-protected operations for concurrent access
- **Resource Cleanup**: Automatic cleanup on destruction or Close()
- **Double Init**: Prevents re-initialization
- **Uninitialized Operations**: Returns errors for operations without initialization

## Testing

The library includes a comprehensive test suite covering:

- All messaging patterns (REQ/REP, PUB/SUB, PUSH/PULL)
- Edge cases (empty messages, large messages, timeouts)
- Error conditions (double init, uninitialized operations)
- Multi-threading scenarios
- Custom endpoints

Run tests:

```bash
cd build
./prj1_test
```

## Distribution

For consumers of this library, distribute:

1. **Shared library**: `libprj1.so` (Linux), `libprj1.dylib` (macOS), `prj1.dll` (Windows)
2. **Public header**: `prj1.h`

Consumers only need these files - ZeroMQ is statically linked into the shared library.

## License

This project integrates ZeroMQ which is licensed under the Mozilla Public License v2.0. See `lib/zeromq/LICENSE` for details.

## Contributing

Contributions are welcome! Please ensure:

- Code follows existing style
- All tests pass
- New features include tests
- Documentation is updated

## Support

For issues or questions, please open an issue on the GitHub repository.
