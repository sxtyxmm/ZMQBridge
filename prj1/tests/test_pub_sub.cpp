// Test PUB/SUB pattern
#include "prj1.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <atomic>

std::atomic<int> received_count{0};

void test_pub_sub_publisher() {
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::PUB_SUB;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_pub_sub.sock";
    cfg.verbose = false;
    
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    assert(err == prj1::ErrorCode::OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for subscriber
    
    for (int i = 0; i < 10; ++i) {
        std::string msg = "Broadcast " + std::to_string(i);
        err = ctx.SendMessage(msg);
        assert(err == prj1::ErrorCode::OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ctx.Close();
}

void test_pub_sub_subscriber() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for publisher
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::PUB_SUB;
    cfg.mode = prj1::Mode::CLIENT;
    cfg.endpoint = "ipc:///tmp/test_pub_sub.sock";
    cfg.recv_timeout_ms = 500;
    cfg.verbose = false;
    
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    assert(err == prj1::ErrorCode::OK);
    
    for (int i = 0; i < 10; ++i) {
        std::string msg;
        err = ctx.ReceiveMessage(msg);
        if (err == prj1::ErrorCode::OK) {
            received_count++;
        }
    }
    
    ctx.Close();
}

int main() {
    std::cout << "Testing PUB/SUB pattern..." << std::endl;
    
    std::thread pub_thread(test_pub_sub_publisher);
    std::thread sub_thread(test_pub_sub_subscriber);
    
    pub_thread.join();
    sub_thread.join();
    
    std::cout << "✓ PUB/SUB pattern test passed! Received " << received_count << " messages" << std::endl;
    assert(received_count > 0); // Should receive at least some messages
    return 0;
}
