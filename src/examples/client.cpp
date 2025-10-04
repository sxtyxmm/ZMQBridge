#include "prj1.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace prj1;

int main(int argc, char* argv[]) {
    std::cout << "=== prj1 Client Example ===" << std::endl;
    
    // Parse command line arguments for pattern
    Pattern pattern = Pattern::REQ_REP;
    if (argc > 1) {
        std::string pattern_str = argv[1];
        if (pattern_str == "pub_sub") {
            pattern = Pattern::PUB_SUB;
        } else if (pattern_str == "push_pull") {
            pattern = Pattern::PUSH_PULL;
        }
    }
    
    std::cout << "Pattern: ";
    switch (pattern) {
        case Pattern::REQ_REP:
            std::cout << "REQ/REP (Request-Reply)" << std::endl;
            break;
        case Pattern::PUB_SUB:
            std::cout << "PUB/SUB (Publish-Subscribe)" << std::endl;
            break;
        case Pattern::PUSH_PULL:
            std::cout << "PUSH/PULL (Pipeline)" << std::endl;
            break;
    }
    
    // Wait a bit for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Create wrapper instance
    ZMQWrapper wrapper;
    
    // Configure as client
    Config config;
    config.pattern = pattern;
    config.mode = Mode::CLIENT;
    config.timeout_ms = 5000;
    config.enable_logging = true;
    
    // Initialize
    ErrorCode result = wrapper.Init(config);
    if (result != ErrorCode::SUCCESS) {
        std::cerr << "Failed to initialize: " 
                  << ZMQWrapper::GetErrorMessage(result) << std::endl;
        return 1;
    }
    
    std::cout << "Client initialized successfully" << std::endl;
    
    // Handle different patterns
    if (pattern == Pattern::REQ_REP) {
        // REQ/REP: Send request, receive reply
        for (int i = 0; i < 5; i++) {
            std::string message = "Hello #" + std::to_string(i);
            std::cout << "Sending: " << message << std::endl;
            
            result = wrapper.SendMessage(message);
            if (result != ErrorCode::SUCCESS) {
                std::cerr << "Failed to send: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
                break;
            }
            
            std::string reply;
            result = wrapper.ReceiveMessage(reply);
            
            if (result == ErrorCode::SUCCESS) {
                std::cout << "Received: " << reply << std::endl;
            } else if (result == ErrorCode::ERROR_TIMEOUT) {
                std::cout << "Timeout waiting for reply" << std::endl;
            } else {
                std::cerr << "Failed to receive: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else if (pattern == Pattern::PUB_SUB) {
        // PUB/SUB: Subscribe and receive messages
        result = wrapper.Subscribe("");  // Subscribe to all messages
        if (result != ErrorCode::SUCCESS) {
            std::cerr << "Failed to subscribe: " 
                      << ZMQWrapper::GetErrorMessage(result) << std::endl;
            return 1;
        }
        
        std::cout << "Subscribed. Receiving messages..." << std::endl;
        
        for (int i = 0; i < 10; i++) {
            std::string message;
            result = wrapper.ReceiveMessage(message);
            
            if (result == ErrorCode::SUCCESS) {
                std::cout << "Received: " << message << std::endl;
            } else if (result == ErrorCode::ERROR_TIMEOUT) {
                std::cout << "Timeout waiting for message" << std::endl;
            } else {
                std::cerr << "Failed to receive: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
                break;
            }
        }
    } else if (pattern == Pattern::PUSH_PULL) {
        // PUSH/PULL: Pull messages (worker)
        std::cout << "Worker ready. Pulling messages..." << std::endl;
        
        for (int i = 0; i < 10; i++) {
            std::string message;
            result = wrapper.ReceiveMessage(message);
            
            if (result == ErrorCode::SUCCESS) {
                std::cout << "Received: " << message << std::endl;
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } else if (result == ErrorCode::ERROR_TIMEOUT) {
                std::cout << "Timeout waiting for task" << std::endl;
            } else {
                std::cerr << "Failed to receive: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
                break;
            }
        }
    }
    
    // Clean up
    std::cout << "Closing client..." << std::endl;
    wrapper.Close();
    
    std::cout << "Client terminated successfully" << std::endl;
    return 0;
}
