// Example: prj1 client
#include "prj1.h"
#include <iostream>

int main() {
    prj1::Config cfg;
    cfg.pattern = prj1::Pattern::REQ_REP;
    cfg.mode = prj1::Mode::CLIENT;
#ifdef _WIN32
    cfg.endpoint = "\\.\pipe\prj1_pipe";
#else
    cfg.endpoint = "ipc:///tmp/prj1.sock";
#endif
    cfg.verbose = true;
    prj1::Context ctx;
    auto err = ctx.Init(cfg);
    if (err != prj1::ErrorCode::OK) {
        std::cerr << "Init failed: " << ctx.GetLastError() << std::endl;
        return 1;
    }
    for (int i = 0; i < 5; ++i) {
        std::string msg = "Hello " + std::to_string(i);
        err = ctx.SendMessage(msg);
        if (err != prj1::ErrorCode::OK) {
            std::cerr << "Send failed: " << ctx.GetLastError() << std::endl;
            continue;
        }
        std::string reply;
        err = ctx.ReceiveMessage(reply);
        if (err != prj1::ErrorCode::OK) {
            std::cerr << "Receive failed: " << ctx.GetLastError() << std::endl;
            continue;
        }
        std::cout << "Reply: " << reply << std::endl;
    }
    ctx.Close();
    return 0;
}
