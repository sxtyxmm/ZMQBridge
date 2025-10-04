#include "prj1.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <cassert>

using namespace prj1;

// Test result tracking
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void name(); \
    struct name##_runner { \
        name##_runner() { \
            std::cout << "\n[TEST] " << #name << std::endl; \
            tests_run++; \
            try { \
                name(); \
                tests_passed++; \
                std::cout << "[PASS] " << #name << std::endl; \
            } catch (const std::exception& e) { \
                tests_failed++; \
                std::cerr << "[FAIL] " << #name << ": " << e.what() << std::endl; \
            } \
        } \
    } name##_instance; \
    void name()

#define ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + message); \
    }

// Helper function to start a server in a separate thread
void RunServer(Pattern pattern, std::atomic<bool>& server_ready, 
               std::atomic<bool>& server_stop, int message_count) {
    ZMQWrapper server;
    Config config;
    config.pattern = pattern;
    config.mode = Mode::SERVER;
    config.timeout_ms = 1000;
    config.enable_logging = false;
    
    ErrorCode result = server.Init(config);
    if (result != ErrorCode::SUCCESS) {
        std::cerr << "Server init failed: " << ZMQWrapper::GetErrorMessage(result) << std::endl;
        return;
    }
    
    server_ready = true;
    
    if (pattern == Pattern::REQ_REP) {
        for (int i = 0; i < message_count && !server_stop; i++) {
            std::string message;
            result = server.ReceiveMessage(message);
            if (result == ErrorCode::SUCCESS) {
                server.SendMessage("Echo: " + message);
            }
        }
    } else if (pattern == Pattern::PUB_SUB) {
        for (int i = 0; i < message_count && !server_stop; i++) {
            server.SendMessage("Message #" + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    } else if (pattern == Pattern::PUSH_PULL) {
        for (int i = 0; i < message_count && !server_stop; i++) {
            server.SendMessage("Task #" + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    server.Close();
}

// Test 1: Basic initialization and cleanup
TEST(test_basic_init_cleanup) {
    ZMQWrapper wrapper;
    
    ASSERT(!wrapper.IsInitialized(), "Should not be initialized initially");
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::SERVER;
    config.timeout_ms = 1000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/test_basic.sock";
    
    ErrorCode result = wrapper.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Init should succeed");
    ASSERT(wrapper.IsInitialized(), "Should be initialized after Init");
    
    result = wrapper.Close();
    ASSERT(result == ErrorCode::SUCCESS, "Close should succeed");
    
    // Double close should be safe
    result = wrapper.Close();
    ASSERT(result == ErrorCode::SUCCESS, "Double close should be safe");
}

// Test 2: REQ/REP pattern
TEST(test_req_rep_pattern) {
    std::atomic<bool> server_ready(false);
    std::atomic<bool> server_stop(false);
    
    std::thread server_thread(RunServer, Pattern::REQ_REP, 
                             std::ref(server_ready), std::ref(server_stop), 3);
    
    // Wait for server to be ready
    while (!server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ZMQWrapper client;
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 2000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/prj1.sock";
    
    ErrorCode result = client.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Client init should succeed");
    
    // Send and receive messages
    for (int i = 0; i < 3; i++) {
        std::string message = "Test message " + std::to_string(i);
        result = client.SendMessage(message);
        ASSERT(result == ErrorCode::SUCCESS, "Send should succeed");
        
        std::string reply;
        result = client.ReceiveMessage(reply);
        ASSERT(result == ErrorCode::SUCCESS, "Receive should succeed");
        ASSERT(reply == "Echo: " + message, "Reply should match expected format");
    }
    
    client.Close();
    server_stop = true;
    server_thread.join();
}

// Test 3: PUB/SUB pattern
TEST(test_pub_sub_pattern) {
    std::atomic<bool> server_ready(false);
    std::atomic<bool> server_stop(false);
    
    std::thread server_thread(RunServer, Pattern::PUB_SUB, 
                             std::ref(server_ready), std::ref(server_stop), 5);
    
    // Wait for server to be ready
    while (!server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ZMQWrapper client;
    Config config;
    config.pattern = Pattern::PUB_SUB;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 2000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/prj1.sock";
    
    ErrorCode result = client.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Client init should succeed");
    
    result = client.Subscribe("");
    ASSERT(result == ErrorCode::SUCCESS, "Subscribe should succeed");
    
    // Wait for slow joiner issue
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Receive messages
    int received_count = 0;
    for (int i = 0; i < 5; i++) {
        std::string message;
        result = client.ReceiveMessage(message);
        if (result == ErrorCode::SUCCESS) {
            received_count++;
        }
    }
    
    ASSERT(received_count > 0, "Should receive at least one message");
    
    client.Close();
    server_stop = true;
    server_thread.join();
}

// Test 4: PUSH/PULL pattern
TEST(test_push_pull_pattern) {
    std::atomic<bool> server_ready(false);
    std::atomic<bool> server_stop(false);
    
    std::thread server_thread(RunServer, Pattern::PUSH_PULL, 
                             std::ref(server_ready), std::ref(server_stop), 5);
    
    // Wait for server to be ready
    while (!server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ZMQWrapper client;
    Config config;
    config.pattern = Pattern::PUSH_PULL;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 2000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/prj1.sock";
    
    ErrorCode result = client.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Client init should succeed");
    
    // Receive messages
    int received_count = 0;
    for (int i = 0; i < 5; i++) {
        std::string message;
        result = client.ReceiveMessage(message);
        if (result == ErrorCode::SUCCESS) {
            received_count++;
        }
    }
    
    ASSERT(received_count == 5, "Should receive all messages");
    
    client.Close();
    server_stop = true;
    server_thread.join();
}

// Test 5: Empty message handling
TEST(test_empty_message) {
    std::atomic<bool> server_ready(false);
    std::atomic<bool> server_stop(false);
    
    std::thread server_thread([&]() {
        ZMQWrapper server;
        Config config;
        config.pattern = Pattern::REQ_REP;
        config.mode = Mode::SERVER;
        config.timeout_ms = 1000;
        config.enable_logging = false;
        config.endpoint = "ipc:///tmp/test_empty.sock";
        
        if (server.Init(config) == ErrorCode::SUCCESS) {
            server_ready = true;
            std::string message;
            if (server.ReceiveMessage(message) == ErrorCode::SUCCESS) {
                server.SendMessage("");  // Send empty reply
            }
        }
        server.Close();
    });
    
    while (!server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ZMQWrapper client;
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 2000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/test_empty.sock";
    
    ErrorCode result = client.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Client init should succeed");
    
    result = client.SendMessage("");
    ASSERT(result == ErrorCode::SUCCESS, "Should be able to send empty message");
    
    std::string reply;
    result = client.ReceiveMessage(reply);
    ASSERT(result == ErrorCode::SUCCESS, "Should receive reply");
    ASSERT(reply.empty(), "Reply should be empty");
    
    client.Close();
    server_stop = true;
    if (server_thread.joinable()) {
        server_thread.join();
    }
}

// Test 6: Large message handling
TEST(test_large_message) {
    std::atomic<bool> server_ready(false);
    std::atomic<bool> server_done(false);
    
    std::thread server_thread([&]() {
        ZMQWrapper server;
        Config config;
        config.pattern = Pattern::REQ_REP;
        config.mode = Mode::SERVER;
        config.timeout_ms = 10000;  // Very long timeout for large messages
        config.enable_logging = false;
        config.endpoint = "ipc:///tmp/test_large.sock";
        
        if (server.Init(config) == ErrorCode::SUCCESS) {
            server_ready = true;
            std::string message;
            if (server.ReceiveMessage(message) == ErrorCode::SUCCESS) {
                server.SendMessage(message);  // Echo back
            }
            // Wait a bit before closing
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        server.Close();
        server_done = true;
    });
    
    while (!server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ZMQWrapper client;
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 10000;  // Very long timeout for large messages
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/test_large.sock";
    
    ErrorCode result = client.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Client init should succeed");
    
    // Create a large message (1 MB)
    std::string large_message(1024 * 1024, 'A');
    result = client.SendMessage(large_message);
    ASSERT(result == ErrorCode::SUCCESS, "Should be able to send large message");
    
    std::string reply;
    result = client.ReceiveMessage(reply);
    ASSERT(result == ErrorCode::SUCCESS, "Should receive reply");
    ASSERT(reply.size() == large_message.size(), "Reply size should match");
    
    client.Close();
    
    // Wait for server to finish
    while (!server_done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
}

// Test 7: Error handling - double init
TEST(test_double_init) {
    ZMQWrapper wrapper;
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::SERVER;
    config.timeout_ms = 1000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/test_double.sock";
    
    ErrorCode result = wrapper.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "First init should succeed");
    
    result = wrapper.Init(config);
    ASSERT(result == ErrorCode::ERROR_ALREADY_INITIALIZED, 
           "Second init should fail with ERROR_ALREADY_INITIALIZED");
    
    wrapper.Close();
}

// Test 8: Error handling - operation without init
TEST(test_operation_without_init) {
    ZMQWrapper wrapper;
    
    std::string message;
    ErrorCode result = wrapper.SendMessage("test");
    ASSERT(result == ErrorCode::ERROR_NOT_INITIALIZED, 
           "Send without init should fail");
    
    result = wrapper.ReceiveMessage(message);
    ASSERT(result == ErrorCode::ERROR_NOT_INITIALIZED, 
           "Receive without init should fail");
}

// Test 9: Timeout handling
TEST(test_timeout) {
    ZMQWrapper wrapper;
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 500;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/test_timeout.sock";
    
    // Try to connect to non-existent server (will succeed)
    ErrorCode result = wrapper.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Init should succeed even without server");
    
    // Try to send (will succeed)
    result = wrapper.SendMessage("test");
    ASSERT(result == ErrorCode::SUCCESS, "Send should succeed");
    
    // Try to receive (should timeout)
    std::string message;
    result = wrapper.ReceiveMessage(message);
    ASSERT(result == ErrorCode::ERROR_TIMEOUT, "Should timeout without server");
    
    wrapper.Close();
}

// Test 10: Multi-threading - concurrent sends
TEST(test_concurrent_sends) {
    std::atomic<bool> server_ready(false);
    std::atomic<bool> server_stop(false);
    std::atomic<int> received_count(0);
    
    std::thread server_thread([&]() {
        ZMQWrapper server;
        Config config;
        config.pattern = Pattern::PUSH_PULL;
        config.mode = Mode::SERVER;
        config.timeout_ms = 100;
        config.enable_logging = false;
        config.endpoint = "ipc:///tmp/test_concurrent.sock";
        
        if (server.Init(config) == ErrorCode::SUCCESS) {
            server_ready = true;
            
            while (!server_stop || received_count < 10) {
                if (server.SendMessage("task") == ErrorCode::SUCCESS) {
                    received_count++;
                }
                if (received_count >= 10) break;
            }
        }
        server.Close();
    });
    
    while (!server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create multiple client threads
    std::vector<std::thread> clients;
    std::atomic<int> total_received(0);
    
    for (int i = 0; i < 2; i++) {
        clients.emplace_back([&]() {
            ZMQWrapper client;
            Config config;
            config.pattern = Pattern::PUSH_PULL;
            config.mode = Mode::CLIENT;
            config.timeout_ms = 500;
            config.enable_logging = false;
            config.endpoint = "ipc:///tmp/test_concurrent.sock";
            
            if (client.Init(config) == ErrorCode::SUCCESS) {
                for (int j = 0; j < 5; j++) {
                    std::string message;
                    if (client.ReceiveMessage(message) == ErrorCode::SUCCESS) {
                        total_received++;
                    }
                }
            }
            client.Close();
        });
    }
    
    for (auto& thread : clients) {
        thread.join();
    }
    
    server_stop = true;
    server_thread.join();
    
    ASSERT(total_received > 0, "Should receive messages from multiple clients");
}

// Test 11: GetErrorMessage
TEST(test_error_messages) {
    std::string msg = ZMQWrapper::GetErrorMessage(ErrorCode::SUCCESS);
    ASSERT(!msg.empty(), "Should have error message for SUCCESS");
    
    msg = ZMQWrapper::GetErrorMessage(ErrorCode::ERROR_NOT_INITIALIZED);
    ASSERT(!msg.empty(), "Should have error message for ERROR_NOT_INITIALIZED");
    
    msg = ZMQWrapper::GetErrorMessage(ErrorCode::ERROR_TIMEOUT);
    ASSERT(!msg.empty(), "Should have error message for ERROR_TIMEOUT");
}

// Test 12: Custom endpoint
TEST(test_custom_endpoint) {
    ZMQWrapper wrapper;
    
    Config config;
    config.pattern = Pattern::REQ_REP;
    config.mode = Mode::SERVER;
    config.timeout_ms = 1000;
    config.enable_logging = false;
    config.endpoint = "ipc:///tmp/custom_endpoint.sock";
    
    ErrorCode result = wrapper.Init(config);
    ASSERT(result == ErrorCode::SUCCESS, "Init with custom endpoint should succeed");
    
    wrapper.Close();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  prj1 Comprehensive Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Tests run automatically via static initialization
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "  Total:  " << tests_run << std::endl;
    std::cout << "  Passed: " << tests_passed << std::endl;
    std::cout << "  Failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (tests_failed == 0) ? 0 : 1;
}
