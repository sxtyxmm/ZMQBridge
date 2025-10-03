#ifndef PRJ1_H
#define PRJ1_H

#include <string>
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

// Error codes for operations
enum class ErrorCode : int {
    SUCCESS = 0,
    ERROR_NOT_INITIALIZED = -1,
    ERROR_ALREADY_INITIALIZED = -2,
    ERROR_INVALID_CONFIG = -3,
    ERROR_SOCKET_CREATE_FAILED = -4,
    ERROR_SOCKET_BIND_FAILED = -5,
    ERROR_SOCKET_CONNECT_FAILED = -6,
    ERROR_SEND_FAILED = -7,
    ERROR_RECEIVE_FAILED = -8,
    ERROR_TIMEOUT = -9,
    ERROR_MESSAGE_TOO_LARGE = -10,
    ERROR_INVALID_PATTERN = -11,
    ERROR_PERMISSION_DENIED = -12,
    ERROR_PATH_TOO_LONG = -13,
    ERROR_UNKNOWN = -99
};

// Communication patterns supported
enum class Pattern {
    REQ_REP,    // Request-Reply pattern
    PUB_SUB,    // Publish-Subscribe pattern
    PUSH_PULL   // Push-Pull pattern (pipeline)
};

// Operating mode
enum class Mode {
    SERVER,     // Creates and binds socket
    CLIENT      // Connects to existing socket
};

// Configuration structure for initializing the wrapper
struct Config {
    Pattern pattern;        // Communication pattern to use
    Mode mode;              // Server or client mode
    std::string endpoint;   // Custom endpoint (optional, empty for default)
    int timeout_ms;         // Timeout for receive operations in milliseconds
    bool enable_logging;    // Enable internal logging
    
    // Constructor with defaults
    Config() 
        : pattern(Pattern::REQ_REP)
        , mode(Mode::SERVER)
        , endpoint("")
        , timeout_ms(5000)
        , enable_logging(false)
    {}
};

// Forward declaration of implementation class (PIMPL pattern for ABI stability)
class ZMQWrapperImpl;

/**
 * @class ZMQWrapper
 * @brief Cross-platform ZeroMQ wrapper providing IPC communication
 * 
 * This class abstracts ZeroMQ functionality and provides a clean API for
 * inter-process communication using Unix Domain Sockets (macOS/Linux) or
 * Named Pipes (Windows). Thread-safe for concurrent operations.
 */
class PRJ1_API ZMQWrapper {
public:
    /**
     * @brief Constructor
     */
    ZMQWrapper();
    
    /**
     * @brief Destructor - automatically cleans up resources
     */
    ~ZMQWrapper();
    
    // Disable copy semantics
    ZMQWrapper(const ZMQWrapper&) = delete;
    ZMQWrapper& operator=(const ZMQWrapper&) = delete;
    
    /**
     * @brief Initialize the wrapper with the given configuration
     * @param config Configuration structure specifying pattern, mode, endpoint, etc.
     * @return ErrorCode indicating success or failure
     * 
     * This function must be called before any other operations.
     * It creates the ZeroMQ context and socket based on the configuration.
     * Handles platform-specific endpoint setup automatically.
     */
    ErrorCode Init(const Config& config);
    
    /**
     * @brief Send a message through the socket
     * @param message The message string to send
     * @return ErrorCode indicating success or failure
     * 
     * Thread-safe. Behavior depends on the pattern:
     * - REQ/REP: Must alternate with ReceiveMessage() in REQ mode
     * - PUB/SUB: Publishes to all subscribers
     * - PUSH/PULL: Pushes to next available worker
     */
    ErrorCode SendMessage(const std::string& message);
    
    /**
     * @brief Receive a message from the socket
     * @param message Output parameter to store received message
     * @return ErrorCode indicating success or failure
     * 
     * Thread-safe. Blocks until message arrives or timeout occurs.
     * Behavior depends on the pattern:
     * - REQ/REP: Must alternate with SendMessage() in REP mode
     * - PUB/SUB: Receives published messages matching subscription
     * - PUSH/PULL: Pulls next available message
     */
    ErrorCode ReceiveMessage(std::string& message);
    
    /**
     * @brief Close the wrapper and clean up all resources
     * @return ErrorCode indicating success or failure
     * 
     * Closes sockets, terminates ZeroMQ context, and removes
     * socket files (Unix domain sockets). Safe to call multiple times.
     */
    ErrorCode Close();
    
    /**
     * @brief Check if the wrapper is initialized
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const;
    
    /**
     * @brief Get human-readable error message for an error code
     * @param code The error code
     * @return String describing the error
     */
    static std::string GetErrorMessage(ErrorCode code);
    
    /**
     * @brief Set subscription topic for PUB/SUB pattern
     * @param topic Topic prefix to subscribe to (empty for all messages)
     * @return ErrorCode indicating success or failure
     * 
     * Only valid in CLIENT mode with PUB_SUB pattern.
     * Must be called after Init() but before ReceiveMessage().
     */
    ErrorCode Subscribe(const std::string& topic = "");

private:
    ZMQWrapperImpl* pImpl;  // PIMPL idiom for implementation hiding
};

} // namespace prj1

#endif // PRJ1_H
