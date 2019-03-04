#include <network/Worker.h>

#include <spdlog/spdlog.h>
#include <uv.h>

#include <network/Connection.h>

namespace Server {
namespace Network {

    // TODO translate c stype callback into class
    template <typename T, typename... Types> struct delegate {

    };

    Worker::Worker(uint32_t id, uint32_t task_limit, uint32_t slow_req_ms_time) : id(id),
    conn_id_seq(0), state(State::kStopped), task_limit(task_limit), slowRequestMillisecondTime(slow_req_ms_time) {}

    Worker::~Worker() = default;

    void Worker::Start(const struct sockaddr_storage &address) {
        // init loop
        int rc = uv_loop_init(&uvLoop);
        if (rc != 0) {
            spdlog::error("Failed to create loop:", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to init uv loop");
        }
        uvLoop.data = this;

       // TODO init signals

       rc = uv_tcp_init_ex(&uvLoop, &uvNetwork, address.ss_family);
       if (rc != 0) {
           spdlog::error("Failed to call uv_tcp_init_ex:", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_tcp_init_ex");
       }
       uvNetwork.data = this;

       rc = uv_tcp_keepalive(&uvNetwork, 1, 60);
       if (rc != 0) {
           spdlog::error("Failed to call uv_tcp_keepalive:", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_tcp_keepalive");
       }

       rc = uv_tcp_bind(&uvNetwork, (const struct sockaddr *)&address, 0);
       if (rc != 0) {
           spdlog::error("Failed to call uv_tcp_bind:", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_tcp_bind");
       }

       // TODO add callback on open connection and start threads

    }

    void Worker::Stop() {
        // TODO send uvStopAsyc
    }

} // Server
} // Network