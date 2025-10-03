// Test REQ/REP pattern
#include "prj1.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

void test_req_rep_server() {
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_req_rep.sock";
    cfg.verbose = false;
    
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    assert(err == prj1::ErrorCode::OK);
    
    for (int i = 0; i < 10; ++i) {
        std::string msg;
        err = ctx.ReceiveMessage(msg);
        assert(err == prj1::ErrorCode::OK);
        err = ctx.SendMessage("ACK: " + msg);
        assert(err == prj1::ErrorCode::OK);
    }
    
    ctx.Close();
}

void test_req_rep_client() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for server
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::CLIENT;
    cfg.endpoint = "ipc:///tmp/test_req_rep.sock";
    cfg.verbose = false;
    
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    assert(err == prj1::ErrorCode::OK);
    
    for (int i = 0; i < 10; ++i) {
        std::string msg = "Test " + std::to_string(i);
        err = ctx.SendMessage(msg);
        assert(err == prj1::ErrorCode::OK);
        
        std::string reply;
        err = ctx.ReceiveMessage(reply);
        assert(err == prj1::ErrorCode::OK);
        assert(reply == "ACK: " + msg);
    }
    
    ctx.Close();
}

int main() {
    std::cout << "Testing REQ/REP pattern..." << std::endl;
    
    std::thread server_thread(test_req_rep_server);
    std::thread client_thread(test_req_rep_client);
    
    server_thread.join();
    client_thread.join();
    
    std::cout << "✓ REQ/REP pattern test passed!" << std::endl;
    return 0;
}
