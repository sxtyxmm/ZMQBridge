#include "prj1.h"
#include <zmq.h>
#include <mutex>
#include <memory>
#include <iostream>
#include <cstring>
#include <atomic>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <errno.h>
#endif

namespace prj1 {

// Maximum message size (10 MB)
constexpr size_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024;

// Maximum path length for socket files
#ifdef _WIN32
constexpr size_t MAX_PATH_LENGTH = 256;
#else
constexpr size_t MAX_PATH_LENGTH = 108;  // Unix domain socket path limit
#endif

/**
 * @class ZMQWrapperImpl
 * @brief Implementation class for ZMQWrapper (PIMPL pattern)
 */
class ZMQWrapperImpl {
public:
    ZMQWrapperImpl() 
        : context(nullptr)
        , socket(nullptr)
        , initialized(false)
        , config()
        , endpoint_path("")
    {}
    
    ~ZMQWrapperImpl() {
        Cleanup();
    }
    
    ErrorCode Init(const Config& cfg) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (initialized) {
            return ErrorCode::ERROR_ALREADY_INITIALIZED;
        }
        
        // Validate configuration
        if (cfg.timeout_ms < 0) {
            return ErrorCode::ERROR_INVALID_CONFIG;
        }
        
        config = cfg;
        
        // Create ZeroMQ context
        context = zmq_ctx_new();
        if (!context) {
            Log("Failed to create ZeroMQ context");
            return ErrorCode::ERROR_SOCKET_CREATE_FAILED;
        }
        
        // Set context options for clean shutdown
        zmq_ctx_set(context, ZMQ_IO_THREADS, 1);
        
        // Determine socket type based on pattern and mode
        int socket_type = GetSocketType(config.pattern, config.mode);
        if (socket_type < 0) {
            zmq_ctx_term(context);
            context = nullptr;
            return ErrorCode::ERROR_INVALID_PATTERN;
        }
        
        // Create socket
        socket = zmq_socket(context, socket_type);
        if (!socket) {
            Log("Failed to create ZeroMQ socket");
            zmq_ctx_term(context);
            context = nullptr;
            return ErrorCode::ERROR_SOCKET_CREATE_FAILED;
        }
        
        // Set socket options
        int linger = 0;  // Don't wait for unsent messages on close
        zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger));
        
        if (config.timeout_ms > 0) {
            zmq_setsockopt(socket, ZMQ_RCVTIMEO, &config.timeout_ms, sizeof(config.timeout_ms));
        }
        
        // Build endpoint
        endpoint_path = BuildEndpoint(config.endpoint);
        if (endpoint_path.empty()) {
            Log("Failed to build endpoint");
            CleanupSocket();
            return ErrorCode::ERROR_INVALID_CONFIG;
        }
        
        if (endpoint_path.length() >= MAX_PATH_LENGTH) {
            Log("Endpoint path too long: " + endpoint_path);
            CleanupSocket();
            return ErrorCode::ERROR_PATH_TOO_LONG;
        }
        
        // Bind or connect based on mode
        if (config.mode == Mode::SERVER) {
            // For Unix domain sockets, remove existing socket file
#ifndef _WIN32
            if (endpoint_path.substr(0, 6) == "ipc://") {
                std::string socket_file = endpoint_path.substr(6);
                if (access(socket_file.c_str(), F_OK) == 0) {
                    Log("Removing existing socket file: " + socket_file);
                    unlink(socket_file.c_str());
                }
            }
#endif
            
            if (zmq_bind(socket, endpoint_path.c_str()) != 0) {
                int err = zmq_errno();
                Log("Failed to bind to " + endpoint_path + ": " + std::string(zmq_strerror(err)));
                CleanupSocket();
                
                if (err == EACCES) {
                    return ErrorCode::ERROR_PERMISSION_DENIED;
                }
                return ErrorCode::ERROR_SOCKET_BIND_FAILED;
            }
            Log("Bound to " + endpoint_path);
        } else {
            // Client mode - connect with retry logic
            int retry_count = 0;
            const int max_retries = 3;
            const int retry_delay_ms = 100;
            
            while (retry_count < max_retries) {
                if (zmq_connect(socket, endpoint_path.c_str()) == 0) {
                    Log("Connected to " + endpoint_path);
                    break;
                }
                
                int err = zmq_errno();
                Log("Connection attempt " + std::to_string(retry_count + 1) + 
                    " failed: " + std::string(zmq_strerror(err)));
                
                retry_count++;
                if (retry_count < max_retries) {
#ifdef _WIN32
                    Sleep(retry_delay_ms);
#else
                    usleep(retry_delay_ms * 1000);
#endif
                }
            }
            
            if (retry_count >= max_retries) {
                Log("Failed to connect after " + std::to_string(max_retries) + " attempts");
                CleanupSocket();
                return ErrorCode::ERROR_SOCKET_CONNECT_FAILED;
            }
        }
        
        initialized = true;
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode SendMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!initialized || !socket) {
            return ErrorCode::ERROR_NOT_INITIALIZED;
        }
        
        if (message.size() > MAX_MESSAGE_SIZE) {
            Log("Message too large: " + std::to_string(message.size()) + " bytes");
            return ErrorCode::ERROR_MESSAGE_TOO_LARGE;
        }
        
        // Allow empty messages
        const char* data = message.empty() ? "" : message.c_str();
        size_t size = message.size();
        
        if (zmq_send(socket, data, size, 0) < 0) {
            int err = zmq_errno();
            Log("Send failed: " + std::string(zmq_strerror(err)));
            return ErrorCode::ERROR_SEND_FAILED;
        }
        
        Log("Sent message: " + std::to_string(size) + " bytes");
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode ReceiveMessage(std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!initialized || !socket) {
            return ErrorCode::ERROR_NOT_INITIALIZED;
        }
        
        // Create a message object
        zmq_msg_t zmq_msg;
        if (zmq_msg_init(&zmq_msg) != 0) {
            Log("Failed to initialize message");
            return ErrorCode::ERROR_RECEIVE_FAILED;
        }
        
        // Receive message
        int nbytes = zmq_msg_recv(&zmq_msg, socket, 0);
        if (nbytes < 0) {
            int err = zmq_errno();
            zmq_msg_close(&zmq_msg);
            
            if (err == EAGAIN || err == ETIMEDOUT) {
                Log("Receive timeout");
                return ErrorCode::ERROR_TIMEOUT;
            }
            
            Log("Receive failed: " + std::string(zmq_strerror(err)));
            return ErrorCode::ERROR_RECEIVE_FAILED;
        }
        
        // Extract data
        size_t size = zmq_msg_size(&zmq_msg);
        if (size > MAX_MESSAGE_SIZE) {
            zmq_msg_close(&zmq_msg);
            Log("Received message too large: " + std::to_string(size) + " bytes");
            return ErrorCode::ERROR_MESSAGE_TOO_LARGE;
        }
        
        const char* data = static_cast<const char*>(zmq_msg_data(&zmq_msg));
        message.assign(data, size);
        
        zmq_msg_close(&zmq_msg);
        
        Log("Received message: " + std::to_string(size) + " bytes");
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode Subscribe(const std::string& topic) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!initialized || !socket) {
            return ErrorCode::ERROR_NOT_INITIALIZED;
        }
        
        if (config.pattern != Pattern::PUB_SUB || config.mode != Mode::CLIENT) {
            Log("Subscribe only valid for PUB/SUB client");
            return ErrorCode::ERROR_INVALID_PATTERN;
        }
        
        if (zmq_setsockopt(socket, ZMQ_SUBSCRIBE, topic.c_str(), topic.length()) != 0) {
            int err = zmq_errno();
            Log("Subscribe failed: " + std::string(zmq_strerror(err)));
            return ErrorCode::ERROR_SOCKET_CREATE_FAILED;
        }
        
        Log("Subscribed to topic: " + (topic.empty() ? "<all>" : topic));
        return ErrorCode::SUCCESS;
    }
    
    ErrorCode Close() {
        std::lock_guard<std::mutex> lock(mutex);
        return Cleanup();
    }
    
    bool IsInitialized() const {
        return initialized;
    }
    
    const Config& GetConfig() const {
        return config;
    }

private:
    void* context;
    void* socket;
    std::atomic<bool> initialized;
    Config config;
    std::string endpoint_path;
    mutable std::mutex mutex;
    
    int GetSocketType(Pattern pattern, Mode mode) const {
        switch (pattern) {
            case Pattern::REQ_REP:
                return (mode == Mode::SERVER) ? ZMQ_REP : ZMQ_REQ;
            case Pattern::PUB_SUB:
                return (mode == Mode::SERVER) ? ZMQ_PUB : ZMQ_SUB;
            case Pattern::PUSH_PULL:
                return (mode == Mode::SERVER) ? ZMQ_PUSH : ZMQ_PULL;
            default:
                return -1;
        }
    }
    
    std::string BuildEndpoint(const std::string& custom_endpoint) const {
        if (!custom_endpoint.empty()) {
            return custom_endpoint;
        }
        
        // Build default platform-specific endpoint
#ifdef _WIN32
        // Windows Named Pipe
        return "ipc://\\\\.\\pipe\\prj1_pipe";
#else
        // Unix Domain Socket
        return "ipc:///tmp/prj1.sock";
#endif
    }
    
    void CleanupSocket() {
        if (socket) {
            zmq_close(socket);
            socket = nullptr;
        }
        if (context) {
            zmq_ctx_term(context);
            context = nullptr;
        }
    }
    
    ErrorCode Cleanup() {
        if (!initialized) {
            return ErrorCode::SUCCESS;
        }
        
        // Remove socket file for Unix domain sockets in server mode
#ifndef _WIN32
        if (config.mode == Mode::SERVER && 
            endpoint_path.substr(0, 6) == "ipc://") {
            std::string socket_file = endpoint_path.substr(6);
            if (access(socket_file.c_str(), F_OK) == 0) {
                Log("Removing socket file: " + socket_file);
                unlink(socket_file.c_str());
            }
        }
#endif
        
        CleanupSocket();
        initialized = false;
        
        Log("Cleanup complete");
        return ErrorCode::SUCCESS;
    }
    
    void Log(const std::string& message) const {
        if (config.enable_logging) {
            std::cout << "[prj1] " << message << std::endl;
        }
    }
};

// ZMQWrapper implementation

ZMQWrapper::ZMQWrapper() 
    : pImpl(new ZMQWrapperImpl())
{}

ZMQWrapper::~ZMQWrapper() {
    delete pImpl;
}

ErrorCode ZMQWrapper::Init(const Config& config) {
    return pImpl->Init(config);
}

ErrorCode ZMQWrapper::SendMessage(const std::string& message) {
    return pImpl->SendMessage(message);
}

ErrorCode ZMQWrapper::ReceiveMessage(std::string& message) {
    return pImpl->ReceiveMessage(message);
}

ErrorCode ZMQWrapper::Close() {
    return pImpl->Close();
}

bool ZMQWrapper::IsInitialized() const {
    return pImpl->IsInitialized();
}

ErrorCode ZMQWrapper::Subscribe(const std::string& topic) {
    return pImpl->Subscribe(topic);
}

std::string ZMQWrapper::GetErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::ERROR_NOT_INITIALIZED:
            return "Wrapper not initialized";
        case ErrorCode::ERROR_ALREADY_INITIALIZED:
            return "Wrapper already initialized";
        case ErrorCode::ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        case ErrorCode::ERROR_SOCKET_CREATE_FAILED:
            return "Failed to create socket";
        case ErrorCode::ERROR_SOCKET_BIND_FAILED:
            return "Failed to bind socket";
        case ErrorCode::ERROR_SOCKET_CONNECT_FAILED:
            return "Failed to connect socket";
        case ErrorCode::ERROR_SEND_FAILED:
            return "Failed to send message";
        case ErrorCode::ERROR_RECEIVE_FAILED:
            return "Failed to receive message";
        case ErrorCode::ERROR_TIMEOUT:
            return "Operation timed out";
        case ErrorCode::ERROR_MESSAGE_TOO_LARGE:
            return "Message too large";
        case ErrorCode::ERROR_INVALID_PATTERN:
            return "Invalid communication pattern";
        case ErrorCode::ERROR_PERMISSION_DENIED:
            return "Permission denied";
        case ErrorCode::ERROR_PATH_TOO_LONG:
            return "Path too long";
        default:
            return "Unknown error";
    }
}

} // namespace prj1
