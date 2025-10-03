// prj1.cpp
// Implementation of prj1 ZeroMQ IPC/UDP wrapper
// Handles cross-platform IPC, static ZeroMQ linking, error handling, and thread safety

#include "prj1.h"
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <chrono>
#include <iostream>

// ZeroMQ headers (vendored)
#include <zmq.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace prj1 {

struct Context::Impl {
    void* zmq_ctx = nullptr;
    void* zmq_sock = nullptr;
    Config config;
    std::string last_error;
    std::mutex mtx;
    std::atomic<bool> initialized{false};
    std::atomic<bool> closed{false};

    // Helper: log if verbose
    void log(const std::string& msg) {
        if (config.verbose) {
            std::cerr << "[prj1] " << msg << std::endl;
        }
    }
};

Context::Context() : pImpl(new Impl) {}
Context::~Context() { Close(); delete pImpl; }

ErrorCode Context::Init(const Config& cfg) {
    std::lock_guard<std::mutex> lock(pImpl->mtx);
    if (pImpl->initialized) return ErrorCode::OK;
    pImpl->config = cfg;
    pImpl->zmq_ctx = zmq_ctx_new();
    if (!pImpl->zmq_ctx) {
        pImpl->last_error = "ZeroMQ context creation failed";
        return ErrorCode::INIT_FAILED;
    }
    int type = ZMQ_REQ;
    switch (cfg.pattern) {
        case Pattern::REQ_REP: type = (cfg.mode == Mode::SERVER) ? ZMQ_REP : ZMQ_REQ; break;
        case Pattern::PUB_SUB: type = (cfg.mode == Mode::SERVER) ? ZMQ_PUB : ZMQ_SUB; break;
        case Pattern::PUSH_PULL: type = (cfg.mode == Mode::SERVER) ? ZMQ_PULL : ZMQ_PUSH; break;
    }
    pImpl->zmq_sock = zmq_socket(pImpl->zmq_ctx, type);
    if (!pImpl->zmq_sock) {
        pImpl->last_error = "ZeroMQ socket creation failed";
        zmq_ctx_term(pImpl->zmq_ctx);
        return ErrorCode::INIT_FAILED;
    }
    zmq_setsockopt(pImpl->zmq_sock, ZMQ_RCVTIMEO, &cfg.recv_timeout_ms, sizeof(int));
    zmq_setsockopt(pImpl->zmq_sock, ZMQ_SNDTIMEO, &cfg.send_timeout_ms, sizeof(int));
    if (cfg.pattern == Pattern::PUB_SUB && cfg.mode == Mode::CLIENT) {
        zmq_setsockopt(pImpl->zmq_sock, ZMQ_SUBSCRIBE, "", 0); // Subscribe to all
    }
    // Platform-specific endpoint checks
#ifdef _WIN32
    if (cfg.endpoint.size() > 256) {
        pImpl->last_error = "Named pipe path too long";
        return ErrorCode::PATH_TOO_LONG;
    }
#else
    if (cfg.endpoint.find("ipc://") == 0) {
        std::string sock_path = cfg.endpoint.substr(6);
        if (sock_path.size() > 108) {
            pImpl->last_error = "Socket path too long";
            return ErrorCode::PATH_TOO_LONG;
        }
        if (cfg.mode == Mode::SERVER) {
            if (access(sock_path.c_str(), F_OK) == 0) {
                if (remove(sock_path.c_str()) != 0) {
                    pImpl->last_error = "Socket file exists and cannot be removed";
                    return ErrorCode::SOCKET_EXISTS;
                }
            }
        }
    }
#endif
    int rc = (cfg.mode == Mode::SERVER)
        ? zmq_bind(pImpl->zmq_sock, cfg.endpoint.c_str())
        : zmq_connect(pImpl->zmq_sock, cfg.endpoint.c_str());
    if (rc != 0) {
        pImpl->last_error = "Failed to bind/connect endpoint: " + std::string(zmq_strerror(zmq_errno()));
        zmq_close(pImpl->zmq_sock);
        zmq_ctx_term(pImpl->zmq_ctx);
        return ErrorCode::INIT_FAILED;
    }
    pImpl->initialized = true;
    pImpl->closed = false;
    pImpl->log("Initialized on endpoint: " + cfg.endpoint);
    return ErrorCode::OK;
}

ErrorCode Context::SendMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(pImpl->mtx);
    if (!pImpl->initialized || pImpl->closed) return ErrorCode::SEND_FAILED;
    if (msg.size() > 1024 * 1024) {
        pImpl->last_error = "Message too large (>1MB)";
        return ErrorCode::MALFORMED_MESSAGE;
    }
    int rc = zmq_send(pImpl->zmq_sock, msg.data(), msg.size(), 0);
    if (rc < 0) {
        pImpl->last_error = "Send failed: " + std::string(zmq_strerror(zmq_errno()));
        return ErrorCode::SEND_FAILED;
    }
    pImpl->log("Sent message: " + msg);
    return ErrorCode::OK;
}

ErrorCode Context::ReceiveMessage(std::string& out_msg) {
    std::lock_guard<std::mutex> lock(pImpl->mtx);
    if (!pImpl->initialized || pImpl->closed) return ErrorCode::RECV_FAILED;
    char buf[1024 * 1024] = {0};
    int rc = zmq_recv(pImpl->zmq_sock, buf, sizeof(buf), 0);
    if (rc < 0) {
        if (zmq_errno() == EAGAIN) {
            pImpl->last_error = "Receive timeout";
            return ErrorCode::RECV_FAILED;
        }
        pImpl->last_error = "Receive failed: " + std::string(zmq_strerror(zmq_errno()));
        return ErrorCode::RECV_FAILED;
    }
    out_msg.assign(buf, rc);
    pImpl->log("Received message: " + out_msg);
    return ErrorCode::OK;
}

ErrorCode Context::Close() {
    std::lock_guard<std::mutex> lock(pImpl->mtx);
    if (pImpl->closed) return ErrorCode::OK;
    if (pImpl->zmq_sock) {
        zmq_close(pImpl->zmq_sock);
        pImpl->zmq_sock = nullptr;
    }
    if (pImpl->zmq_ctx) {
        zmq_ctx_term(pImpl->zmq_ctx);
        pImpl->zmq_ctx = nullptr;
    }
#ifndef _WIN32
    if (pImpl->config.endpoint.find("ipc://") == 0 && pImpl->config.mode == Mode::SERVER) {
        std::string sock_path = pImpl->config.endpoint.substr(6);
        remove(sock_path.c_str());
    }
#endif
    pImpl->closed = true;
    pImpl->log("Closed context and cleaned up");
    return ErrorCode::OK;
}

std::string Context::GetLastError() const {
    std::lock_guard<std::mutex> lock(pImpl->mtx);
    return pImpl->last_error;
}

} // namespace prj1
