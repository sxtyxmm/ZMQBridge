// Test edge cases and error handling
#include "prj1.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <string>

void test_empty_message() {
    std::cout << "  Testing empty message..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_empty.sock";
    cfg.verbose = false;
    
    prj1::Context server;
    assert(server.Init(cfg) == prj1::ErrorCode::OK);
    
    std::thread client_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        prj1::Config client_cfg = cfg;
        client_cfg.mode = prj1::Mode::CLIENT;
        prj1::Context client;
        assert(client.Init(client_cfg) == prj1::ErrorCode::OK);
        assert(client.SendMessage("") == prj1::ErrorCode::OK);
        std::string reply;
        assert(client.ReceiveMessage(reply) == prj1::ErrorCode::OK);
        client.Close();
    });
    
    std::string msg;
    assert(server.ReceiveMessage(msg) == prj1::ErrorCode::OK);
    assert(msg.empty());
    assert(server.SendMessage("OK") == prj1::ErrorCode::OK);
    
    client_thread.join();
    server.Close();
    std::cout << "  ✓ Empty message test passed" << std::endl;
}

void test_large_message() {
    std::cout << "  Testing large message..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_large.sock";
    cfg.verbose = false;
    
    prj1::Context server;
    assert(server.Init(cfg) == prj1::ErrorCode::OK);
    
    std::string large_msg(1024 * 100, 'X'); // 100KB message
    
    std::thread client_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        prj1::Config client_cfg = cfg;
        client_cfg.mode = prj1::Mode::CLIENT;
        prj1::Context client;
        assert(client.Init(client_cfg) == prj1::ErrorCode::OK);
        assert(client.SendMessage(large_msg) == prj1::ErrorCode::OK);
        std::string reply;
        assert(client.ReceiveMessage(reply) == prj1::ErrorCode::OK);
        client.Close();
    });
    
    std::string msg;
    assert(server.ReceiveMessage(msg) == prj1::ErrorCode::OK);
    assert(msg.size() == large_msg.size());
    assert(server.SendMessage("OK") == prj1::ErrorCode::OK);
    
    client_thread.join();
    server.Close();
    std::cout << "  ✓ Large message test passed" << std::endl;
}

void test_too_large_message() {
    std::cout << "  Testing message size limit..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::CLIENT;
    cfg.endpoint = "ipc:///tmp/test_toolarge.sock";
    cfg.verbose = false;
    
    prj1::Context client;
    assert(client.Init(cfg) == prj1::ErrorCode::OK);
    
    std::string huge_msg(1024 * 1024 + 1, 'X'); // >1MB message
    auto err = client.SendMessage(huge_msg);
    assert(err == prj1::ErrorCode::MALFORMED_MESSAGE);
    
    client.Close();
    std::cout << "  ✓ Message size limit test passed" << std::endl;
}

void test_socket_cleanup() {
    std::cout << "  Testing socket cleanup..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_cleanup.sock";
    cfg.verbose = false;
    
    // First instance
    {
        prj1::Context ctx1;
        assert(ctx1.Init(cfg) == prj1::ErrorCode::OK);
        ctx1.Close();
    }
    
    // Second instance should be able to bind to same endpoint
    {
        prj1::Context ctx2;
        assert(ctx2.Init(cfg) == prj1::ErrorCode::OK);
        ctx2.Close();
    }
    
    std::cout << "  ✓ Socket cleanup test passed" << std::endl;
}

void test_timeout() {
    std::cout << "  Testing receive timeout..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::CLIENT;
    cfg.endpoint = "ipc:///tmp/test_timeout.sock";
    cfg.recv_timeout_ms = 100;
    cfg.verbose = false;
    
    prj1::Context client;
    assert(client.Init(cfg) == prj1::ErrorCode::OK);
    
    std::string msg;
    auto err = client.ReceiveMessage(msg); // Should timeout (no server)
    assert(err == prj1::ErrorCode::RECV_FAILED);
    
    client.Close();
    std::cout << "  ✓ Timeout test passed" << std::endl;
}

void test_double_init() {
    std::cout << "  Testing double initialization..." << std::endl;
    
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::SERVER;
    cfg.endpoint = "ipc:///tmp/test_double_init.sock";
    cfg.verbose = false;
    
    prj1::Context ctx;
    assert(ctx.Init(cfg) == prj1::ErrorCode::OK);
    assert(ctx.Init(cfg) == prj1::ErrorCode::OK); // Should return OK (already initialized)
    
    ctx.Close();
    std::cout << "  ✓ Double initialization test passed" << std::endl;
}

void test_send_before_init() {
    std::cout << "  Testing send before init..." << std::endl;
    
    prj1::Context ctx;
    auto err = ctx.SendMessage("Test");
    assert(err == prj1::ErrorCode::SEND_FAILED);
    
    std::cout << "  ✓ Send before init test passed" << std::endl;
}

int main() {
    std::cout << "Testing edge cases and error handling..." << std::endl;
    
    test_empty_message();
    test_large_message();
    test_too_large_message();
    test_socket_cleanup();
    test_timeout();
    test_double_init();
    test_send_before_init();
    
    std::cout << "✓ All edge case tests passed!" << std::endl;
    return 0;
}
