#include <chrono>
#include <iostream>
#include <sstream>
#include <uv.h>

#include "spdlog/spdlog.h"

#include <config/ServerConfig.h>
#include <config/Config.h>
#include <network/Server.h>


using namespace Server;

typedef struct {
    Config cfg;
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<Network::Server> server;
} Application;

// Handle all signals catched
void signal_handler_stop_app(uv_signal_t *handle, int signum) {
    Application *pApp = static_cast<Application *>(handle->data);
    spdlog::info("Receive stop signal");
    uv_stop(handle->loop);
}

int main(int argc, char **argv) {
    Application app;
    spdlog::info("Starting application");
    spdlog::set_level(spdlog::level::debug);
    app.server = std::make_shared<Network::Server>();
    uv_loop_t loop;
    uv_loop_init(&loop);

    uv_signal_t sig_stop;
    uv_signal_init(&loop, &sig_stop);
    uv_signal_start(&sig_stop, signal_handler_stop_app, SIGTERM);
    sig_stop.data = &app;

    try {
        spdlog::info("Start application\n");
        app.server->Start();
        uv_run(&loop, UV_RUN_DEFAULT);
        app.server->Stop();
    } catch (std::exception &e) {
        spdlog::error("Fatal error: {}", e.what());
    }
    return 0;
}