#include "prj1.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace prj1;

int main(int argc, char* argv[]) {
    std::cout << "=== prj1 Server Example ===" << std::endl;
    
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
    
    // Create wrapper instance
    ZMQWrapper wrapper;
    
    // Configure as server
    Config config;
    config.pattern = pattern;
    config.mode = Mode::SERVER;
    config.timeout_ms = 5000;
    config.enable_logging = true;
    
    // Initialize
    ErrorCode result = wrapper.Init(config);
    if (result != ErrorCode::SUCCESS) {
        std::cerr << "Failed to initialize: " 
                  << ZMQWrapper::GetErrorMessage(result) << std::endl;
        return 1;
    }
    
    std::cout << "Server initialized successfully" << std::endl;
    std::cout << "Waiting for messages..." << std::endl;
    
    // Handle different patterns
    if (pattern == Pattern::REQ_REP) {
        // REQ/REP: Receive request, send reply
        for (int i = 0; i < 10; i++) {
            std::string message;
            result = wrapper.ReceiveMessage(message);
            
            if (result == ErrorCode::SUCCESS) {
                std::cout << "Received: " << message << std::endl;
                
                // Send reply
                std::string reply = "Echo: " + message;
                result = wrapper.SendMessage(reply);
                
                if (result == ErrorCode::SUCCESS) {
                    std::cout << "Sent reply: " << reply << std::endl;
                } else {
                    std::cerr << "Failed to send reply: " 
                              << ZMQWrapper::GetErrorMessage(result) << std::endl;
                }
            } else if (result == ErrorCode::ERROR_TIMEOUT) {
                std::cout << "Timeout waiting for message" << std::endl;
            } else {
                std::cerr << "Failed to receive: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
                break;
            }
        }
    } else if (pattern == Pattern::PUB_SUB) {
        // PUB/SUB: Publish messages
        std::cout << "Publishing messages (press Ctrl+C to stop)..." << std::endl;
        
        for (int i = 0; i < 20; i++) {
            std::string message = "Message #" + std::to_string(i);
            result = wrapper.SendMessage(message);
            
            if (result == ErrorCode::SUCCESS) {
                std::cout << "Published: " << message << std::endl;
            } else {
                std::cerr << "Failed to publish: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } else if (pattern == Pattern::PUSH_PULL) {
        // PUSH/PULL: Push messages to workers
        std::cout << "Pushing messages to workers..." << std::endl;
        
        for (int i = 0; i < 10; i++) {
            std::string message = "Task #" + std::to_string(i);
            result = wrapper.SendMessage(message);
            
            if (result == ErrorCode::SUCCESS) {
                std::cout << "Pushed: " << message << std::endl;
            } else {
                std::cerr << "Failed to push: " 
                          << ZMQWrapper::GetErrorMessage(result) << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Clean up
    std::cout << "Closing server..." << std::endl;
    wrapper.Close();
    
    std::cout << "Server terminated successfully" << std::endl;
    return 0;
}
