// Test PUSH/PULL pattern
#include "prj1.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

void test_push_pull_puller() {
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::PUSH_PULL;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_push_pull.sock";
    cfg.verbose = false;
    
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    assert(err == prj1::ErrorCode::OK);
    
    for (int i = 0; i < 10; ++i) {
        std::string msg;
        err = ctx.ReceiveMessage(msg);
        assert(err == prj1::ErrorCode::OK);
        assert(msg.find("Task") != std::string::npos);
    }
    
    ctx.Close();
}

void test_push_pull_pusher() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for puller
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::PUSH_PULL;
    cfg.mode = prj1::Mode::CLIENT;
    cfg.endpoint = "ipc:///tmp/test_push_pull.sock";
    cfg.verbose = false;
    
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    assert(err == prj1::ErrorCode::OK);
    
    for (int i = 0; i < 10; ++i) {
        std::string msg = "Task " + std::to_string(i);
        err = ctx.SendMessage(msg);
        assert(err == prj1::ErrorCode::OK);
    }
    
    ctx.Close();
}

int main() {
    std::cout << "Testing PUSH/PULL pattern..." << std::endl;
    
    std::thread pull_thread(test_push_pull_puller);
    std::thread push_thread(test_push_pull_pusher);
    
    pull_thread.join();
    push_thread.join();
    
    std::cout << "✓ PUSH/PULL pattern test passed!" << std::endl;
    return 0;
}
