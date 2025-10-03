# ZMQBridge Usage Guide

## Quick Start

### 1. Build the Library

```bash
# Clone and enter directory
git clone https://github.com/sxtyxmm/ZMQBridge.git
cd ZMQBridge

# Download ZeroMQ (first time only)
./setup_zeromq.sh

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests to verify
./prj1_test
```

### 2. Link to Your Project

#### Using CMake

```cmake
# Find the library
find_library(PRJ1_LIBRARY prj1 HINTS /path/to/ZMQBridge/build)

# Add include directory
include_directories(/path/to/ZMQBridge/include)

# Link your executable
add_executable(myapp main.cpp)
target_link_libraries(myapp ${PRJ1_LIBRARY} pthread)
```

#### Manual Compilation

```bash
g++ -std=c++14 myapp.cpp -I/path/to/ZMQBridge/include -L/path/to/ZMQBridge/build -lprj1 -lpthread -o myapp
```

## Pattern Selection Guide

### When to Use REQ/REP (Request-Reply)

**Best For:**
- Client-server interactions
- RPC-style communication
- When you need a response for every request
- Synchronous operations

**Example Use Cases:**
- Database query/response
- API calls
- Configuration requests
- Status checks

**Characteristics:**
- Synchronous (blocking)
- One-to-one communication
- Must alternate send/receive
- Guaranteed order

### When to Use PUB/SUB (Publish-Subscribe)

**Best For:**
- Broadcasting messages to multiple clients
- Event notifications
- Real-time data feeds
- Loosely coupled systems

**Example Use Cases:**
- Stock market updates
- System monitoring alerts
- Chat room messages
- Sensor data distribution

**Characteristics:**
- Asynchronous
- One-to-many communication
- No acknowledgment (fire and forget)
- Subscribers can filter by topic
- Beware of "slow joiner" problem

### When to Use PUSH/PULL (Pipeline)

**Best For:**
- Load distribution
- Work queue systems
- Parallel task processing
- Stream processing

**Example Use Cases:**
- Task distribution to workers
- Image processing pipeline
- Log aggregation
- Batch job processing

**Characteristics:**
- Asynchronous
- Load balanced (round-robin)
- One-to-many (but load distributed)
- No acknowledgment

## Common Patterns

### Pattern 1: Simple Echo Server

```cpp
#include "prj1.h"
#include <iostream>

using namespace prj1;

int main() {
    ZMQWrapper server;
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::SERVER;
    config.enable_logging = true;
    
    if (server.Init(config) != ErrorCode::SUCCESS) {
        return 1;
    }
    
    while (true) {
        std::string request;
        ErrorCode result = server.ReceiveMessage(request);
        
        if (result == ErrorCode::SUCCESS) {
            std::cout << "Received: " << request << std::endl;
            server.SendMessage("Echo: " + request);
        } else if (result == ErrorCode::ERROR_TIMEOUT) {
            continue;  // Timeout, try again
        } else {
            break;  // Error, exit
        }
    }
    
    server.Close();
    return 0;
}
```

### Pattern 2: Publisher with Topics

```cpp
#include "prj1.h"
#include <iostream>
#include <thread>

using namespace prj1;

int main() {
    ZMQWrapper publisher;
    
    Config config;
    config.pattern = Pattern::PUB_SUB;
    config.mode = Mode::SERVER;
    
    publisher.Init(config);
    
    // Give subscribers time to connect
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    while (true) {
        publisher.SendMessage("TEMP:25.5");
        publisher.SendMessage("HUMIDITY:60.3");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    publisher.Close();
    return 0;
}
```

### Pattern 3: Worker Pool

**Master (PUSH):**

```cpp
#include "prj1.h"
#include <iostream>

using namespace prj1;

int main() {
    ZMQWrapper master;
    
    Config config;
    config.pattern = Pattern::PUSH_PULL;
    config.mode = Mode::SERVER;
    
    master.Init(config);
    
    // Distribute 100 tasks
    for (int i = 0; i < 100; i++) {
        std::string task = "Task_" + std::to_string(i);
        master.SendMessage(task);
        std::cout << "Pushed: " << task << std::endl;
    }
    
    master.Close();
    return 0;
}
```

**Worker (PULL):**

```cpp
#include "prj1.h"
#include <iostream>
#include <thread>

using namespace prj1;

void process_task(const std::string& task) {
    // Simulate work
    std::cout << "Processing: " << task << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main(int argc, char* argv[]) {
    int worker_id = (argc > 1) ? std::stoi(argv[1]) : 0;
    
    ZMQWrapper worker;
    
    Config config;
    config.pattern = Pattern::PUSH_PULL;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 1000;
    
    worker.Init(config);
    
    std::cout << "Worker " << worker_id << " started" << std::endl;
    
    while (true) {
        std::string task;
        ErrorCode result = worker.ReceiveMessage(task);
        
        if (result == ErrorCode::SUCCESS) {
            process_task(task);
        } else if (result == ErrorCode::ERROR_TIMEOUT) {
            continue;
        } else {
            break;
        }
    }
    
    worker.Close();
    return 0;
}
```

## Best Practices

### 1. Always Check Return Codes

```cpp
ErrorCode result = wrapper.Init(config);
if (result != ErrorCode::SUCCESS) {
    std::cerr << "Error: " << ZMQWrapper::GetErrorMessage(result) << std::endl;
    return 1;
}
```

### 2. Handle Timeouts Gracefully

```cpp
std::string message;
ErrorCode result = wrapper.ReceiveMessage(message);

if (result == ErrorCode::ERROR_TIMEOUT) {
    // This is expected, retry or handle accordingly
    continue;
} else if (result != ErrorCode::SUCCESS) {
    // This is an actual error
    std::cerr << "Error: " << ZMQWrapper::GetErrorMessage(result) << std::endl;
    break;
}
```

### 3. Use Appropriate Timeouts

```cpp
// For interactive applications
config.timeout_ms = 1000;  // 1 second

// For batch processing
config.timeout_ms = 30000;  // 30 seconds

// For long-running operations
config.timeout_ms = 0;  // No timeout (blocking)
```

### 4. Enable Logging During Development

```cpp
config.enable_logging = true;  // See internal operations
```

### 5. Clean Up Resources

```cpp
// Wrapper automatically cleans up in destructor
// But you can explicitly close if needed
wrapper.Close();
```

### 6. Custom Endpoints for Isolation

```cpp
// Test environment
config.endpoint = "ipc:///tmp/myapp_test.sock";

// Production environment
config.endpoint = "ipc:///tmp/myapp_prod.sock";
```

## Troubleshooting

### Connection Refused / Bind Failed

**Problem:** Client can't connect or server can't bind.

**Solutions:**
- Check if socket file already exists (Unix)
- Verify permissions on /tmp directory
- Ensure server starts before client
- Check for port/pipe conflicts (Windows)

### Timeout Errors

**Problem:** Receiving `ERROR_TIMEOUT` frequently.

**Solutions:**
- Increase timeout value in config
- Check if server is running
- Verify network connectivity
- For PUB/SUB, account for slow joiner problem

### PUB/SUB Missing Messages

**Problem:** Subscriber not receiving published messages.

**Solutions:**
- Start subscriber before publisher
- Add delay after Init() for connection setup
- Use Subscribe() to filter topics
- Remember: PUB/SUB is fire-and-forget

### Permission Denied

**Problem:** `ERROR_PERMISSION_DENIED` error.

**Solutions:**
- Check /tmp directory permissions
- Run with appropriate user privileges
- Use custom endpoint in writable location

### Large Message Failures

**Problem:** Large messages fail to send/receive.

**Solutions:**
- Check message size (max 10MB)
- Increase timeout for large messages
- Consider splitting large messages
- Verify available memory

## Performance Tips

### 1. Reuse Wrapper Instances

```cpp
// Good: Reuse wrapper
ZMQWrapper wrapper;
wrapper.Init(config);
for (int i = 0; i < 1000; i++) {
    wrapper.SendMessage("message");
}
wrapper.Close();

// Bad: Create new wrapper each time
for (int i = 0; i < 1000; i++) {
    ZMQWrapper wrapper;
    wrapper.Init(config);
    wrapper.SendMessage("message");
    wrapper.Close();
}
```

### 2. Batch Messages When Possible

```cpp
// For PUSH/PULL, send tasks in batches
for (int i = 0; i < 100; i++) {
    pusher.SendMessage("task_" + std::to_string(i));
}
```

### 3. Adjust Timeout Based on Workload

```cpp
// Heavy processing
config.timeout_ms = 60000;  // 1 minute

// Light processing
config.timeout_ms = 1000;   // 1 second
```

### 4. Use Multiple Workers

```bash
# Start multiple workers for parallel processing
./worker 1 &
./worker 2 &
./worker 3 &
./worker 4 &
```

## Security Considerations

1. **Endpoint Security**: Unix domain sockets use file permissions
2. **Message Validation**: Always validate received messages
3. **Resource Limits**: Be aware of message size limits (10MB)
4. **Error Handling**: Don't expose internal error details to clients

## Migration Guide

### From Raw ZeroMQ

```cpp
// Before (raw ZeroMQ)
void* context = zmq_ctx_new();
void* socket = zmq_socket(context, ZMQ_REP);
zmq_bind(socket, "ipc:///tmp/server.sock");

// After (prj1)
ZMQWrapper server;
Config config;
config.pattern = Pattern::REQ_REP;
config.mode = Mode::SERVER;
config.endpoint = "ipc:///tmp/server.sock";
server.Init(config);
```

### From Socket Programming

```cpp
// Before (TCP sockets)
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
bind(sockfd, ...);
listen(sockfd, ...);
accept(sockfd, ...);

// After (prj1 - much simpler!)
ZMQWrapper server;
Config config;
config.pattern = Pattern::REQ_REP;
config.mode = Mode::SERVER;
server.Init(config);
```

## Additional Resources

- [ZeroMQ Guide](http://zguide.zeromq.org/) - Comprehensive guide to ZeroMQ patterns
- [API Reference](../include/prj1.h) - Complete API documentation in header
- [Examples](../src/examples/) - Working examples for all patterns
- [Tests](../src/test/test_suite.cpp) - Test suite showing various use cases
