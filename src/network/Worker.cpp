#include <network/Worker.h>

#include <spdlog/spdlog.h>
#include <uv.h>

#include <network/Connection.h>

namespace Server {
namespace Network {

    // TODO translate c stype callback into class
    template <typename T, typename... Types> struct delegate {
        template <void (T::*TMethod)(uv_handle_t *, Types...)> static void callback(uv_handle_t *self, Types... args) {
            T *instance = static_cast<T *>(self->data);
            (instance->*TMethod)(self, std::forward<Types>(args)...);
        }

        template <void (T::*TMethod)(uv_stream_t *, Types...)> static void callback(uv_stream_t *self, Types... args) {
            T *instance = static_cast<T *>(self->data);
            (instance->*TMethod)(self, std::forward<Types>(args)...);
        }

        template <void (T::*TMethod)(uv_write_t *, Types...)> static void callback(uv_write_t *self, Types... args) {
            T *instance = static_cast<T *>(self->data);
            (instance->*TMethod)(self, std::forward<Types>(args)...);
        }

        template <void (T::*TMethod)(uv_work_t *, Types...)> static void callback(uv_work_t *self, Types... args) {
            T *instance = static_cast<T *>(self->data);
            (instance->*TMethod)(self, std::forward<Types>(args)...);
        }

        template <void (T::*TMethod)(uv_async_t *, Types...)> static void callback(uv_async_t *self, Types... args) {
            T *instance = static_cast<T *>(self->data);
            (instance->*TMethod)(self, std::forward<Types>(args)...);
        }

        template <void (T::*TMethod)(uv_fs_t *, Types...)> static void callback(uv_fs_t *self, Types... args) {
            T *instance = static_cast<T *>(self->data);
            (instance->*TMethod)(self, std::forward<Types>(args)...);
        }

        template <void (T::*TMethod)(Types...)> static void callback(Types... args, void *data) {
            T *instance = static_cast<T *>(data);
            (instance->*TMethod)(std::forward<Types>(args)...);
        }
    };

    Worker::Worker(uint32_t id, uint32_t task_limit, uint32_t slow_req_ms_time) : id(id),
    conn_id_seq(0), state(State::kStopped), task_limit(task_limit), slowRequestMillisecondTime(slow_req_ms_time) {}

    Worker::~Worker() = default;

    void noop(uv_signal_t *handle, int signum) {}

    void Worker::Start(const struct sockaddr_storage &address) {
        spdlog::info("Start worker {}", this->id);
        // Init loop
        int rc = uv_loop_init(&uvLoop);
        if (rc != 0) {
            spdlog::error("Failed to create loop: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to init uv loop");
        }
        uvLoop.data = this;

       // Init signals
       rc = uv_async_init(&uvLoop, &uvStopAsync, delegate<Worker>::callback<&Worker::OnStop>);
       if (rc != 0) {
           spdlog::error("Failed to call uv_async_init: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_async_init");
       }
       uvStopAsync.data = this;

       rc = uv_signal_init(&uvLoop, &uvSigPipe);
       if (rc != 0) {
           spdlog::error("Failed to call uv_async_init: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_async_init");
       }
       uv_signal_start(&uvSigPipe, noop, SIGPIPE);

       // Setup network
       rc = uv_tcp_init_ex(&uvLoop, &uvNetwork, address.ss_family);
       if (rc != 0) {
           spdlog::error("Failed to call uv_tcp_init_ex: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_tcp_init_ex");
       }
       uvNetwork.data = this;

       // Configure network
       int fd;
       rc = uv_fileno((uv_handle_t *)&uvNetwork, &fd);
        if (rc != 0) {
            spdlog::error("Failed to call uv_fileno: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_fileno");
       }

       rc = uv_tcp_keepalive(&uvNetwork, 1, 60);
       if (rc != 0) {
           spdlog::error("Failed to call uv_tcp_keepalive: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_tcp_keepalive");
       }

       int on = 1;
       rc = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
        if (rc != 0) {
            spdlog::error("Failed to call uv_fileno: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_fileno");
       }

       rc = uv_tcp_bind(&uvNetwork, (const struct sockaddr *)&address, 0);
       if (rc != 0) {
           spdlog::error("Failed to call uv_tcp_bind: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
           throw std::runtime_error("Failed to call uv_tcp_bind");
       }

       rc = uv_listen((uv_stream_t *)&uvNetwork, 511, delegate<Worker, int>::callback<&Worker::OnConnectionOpen>);
       if (rc != 0) {
            spdlog::error("Failed to call uv_listen: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_listen");
        }

        // Start thread
        state = State::kRun;
        openHandlersCount = 3; // uvStopAsync, uvSigPipe, uvNetwork
        rc = uv_thread_create(&thread, delegate<Worker>::callback<&Worker::OnRun>, static_cast<void *>(this));
        if (rc != 0) {
            spdlog::error("Failed to call uv_thread_create: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_thread_create");
        }
    }

    void Worker::Stop() {
        uv_async_send(&uvStopAsync);
        int rc = uv_thread_join(&thread);
        if (rc != 0) {
            spdlog::error("Failed to call uv_thread_join: [{}({})]: {}\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_thread_join");
        }
    }

    void Worker::OnRun() {
        spdlog::info("Worker {} start", id);

        uv_run(&uvLoop, UV_RUN_DEFAULT);

        spdlog::info("Worker {} stopped", id);
    }

    void Worker::OnStop(uv_async_t *async) {
        assert(async);
        spdlog::info("On stop", id);
        if (uv_is_closing((uv_handle_t *)&uvStopAsync) == 0) {
            uv_close((uv_handle_t *)&uvStopAsync, delegate<Worker>::callback<&Worker::OnHandleClosed>);
        }
    }

    void Worker::OnConnectionOpen(uv_stream_t *, int) {
        spdlog::info("Connection open in worker {}", id);
        // TODO relese it
    }

    void Worker::OnHandleClosed(uv_handle_t *) {
        // TODO release it
    }

    void Worker::TryCloseConnection(Server::Network::Connection *pconn) {
        // TODO release it
    }

    void Worker::OnStopping(uv_handle_t *handle, void *args) {
        if (handle->type != UV_TCP || (void *)handle == (void *)&uvNetwork) {
            return;
        }
        assert((void *)handle != (void *)&uvStopAsync);
        assert((void *)handle != (void *)&uvNetwork);

        Connection *pconn = (Connection *)handle;
        // TODO check count of task in connection
        spdlog::info("Stopping connection in OnStopping");
        TryCloseConnection(pconn);
    }

    void Worker::ToStopping() {
        state = State::kStopped;

        if (uv_is_closing((uv_handle_t *)&uvNetwork) == 0) {
            uv_close((uv_handle_t *)&uvNetwork, delegate<Worker>::callback<&Worker::OnHandleClosed>);
        }

        uv_walk(&uvLoop, delegate<Worker, void *>::callback<&Worker::OnStopping>, nullptr);
    }

} // Server
} // Network