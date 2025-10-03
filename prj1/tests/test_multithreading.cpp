// Test multi-threading safety
#include "prj1.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <atomic>

std::atomic<int> success_count{0};

void test_concurrent_clients() {
    std::cout << "  Testing concurrent clients..." << std::endl;
    
    // Start server
    std::thread server_thread([]() {
        prj1::Config cfg;
        cfg.pattern = prj1::Pattern::REQ_REP;
        cfg.mode = prj1::Mode::SERVER;
        cfg.endpoint = "ipc:///tmp/test_concurrent.sock";
        cfg.verbose = false;
        
        prj1::Context server;
        assert(server.Init(cfg) == prj1::ErrorCode::OK);
        
        for (int i = 0; i < 50; ++i) { // Handle 50 requests
            std::string msg;
            auto err = server.ReceiveMessage(msg);
            if (err == prj1::ErrorCode::OK) {
                server.SendMessage("ACK: " + msg);
            }
        }
        
        server.Close();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for server
    
    // Start 10 concurrent clients
    std::vector<std::thread> clients;
    for (int i = 0; i < 10; ++i) {
        clients.emplace_back([i]() {
            prj1::Config cfg;
            cfg.pattern = prj1::Pattern::REQ_REP;
            cfg.mode = prj1::Mode::CLIENT;
            cfg.endpoint = "ipc:///tmp/test_concurrent.sock";
            cfg.verbose = false;
            
            prj1::Context client;
            if (client.Init(cfg) != prj1::ErrorCode::OK) return;
            
            for (int j = 0; j < 5; ++j) {
                std::string msg = "Client" + std::to_string(i) + "_Msg" + std::to_string(j);
                if (client.SendMessage(msg) == prj1::ErrorCode::OK) {
                    std::string reply;
                    if (client.ReceiveMessage(reply) == prj1::ErrorCode::OK) {
                        success_count++;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            client.Close();
        });
    }
    
    for (auto& t : clients) {
        t.join();
    }
    
    server_thread.join();
    
    std::cout << "  ✓ Concurrent clients test passed (" << success_count << " successful exchanges)" << std::endl;
    assert(success_count > 40); // Most should succeed
}

void test_thread_safety() {
    std::cout << "  Testing thread safety of Context..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::PUSH_PULL;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_thread_safe.sock";
    cfg.verbose = false;
    
    prj1::Context server;
    assert(server.Init(cfg) == prj1::ErrorCode::OK);
    
    std::atomic<int> received{0};
    
    // Multiple threads trying to receive from same context
    std::vector<std::thread> receivers;
    for (int i = 0; i < 3; ++i) {
        receivers.emplace_back([&]() {
            for (int j = 0; j < 5; ++j) {
                std::string msg;
                if (server.ReceiveMessage(msg) == prj1::ErrorCode::OK) {
                    received++;
                }
            }
        });
    }
    
    // Sender thread
    std::thread sender_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        prj1::Config client_cfg = cfg;
        client_cfg.mode = prj1::Mode::CLIENT;
        prj1::Context client;
        assert(client.Init(client_cfg) == prj1::ErrorCode::OK);
        
        for (int i = 0; i < 15; ++i) {
            client.SendMessage("Msg " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        
        client.Close();
    });
    
    for (auto& t : receivers) {
        t.join();
    }
    sender_thread.join();
    
    server.Close();
    
    std::cout << "  ✓ Thread safety test passed (received " << received << " messages)" << std::endl;
    assert(received == 15); // All messages should be received
}

int main() {
    std::cout << "Testing multi-threading scenarios..." << std::endl;
    
    test_concurrent_clients();
    test_thread_safety();
    
    std::cout << "✓ All multi-threading tests passed!" << std::endl;
    return 0;
}
