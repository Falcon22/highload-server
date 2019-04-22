#include <network/Worker.h>

#include <iostream>

#include <cassert>
#include <chrono>
#include <arpa/inet.h>
#include <iomanip>
#include <memory>
#include <csignal>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <type_traits>
#include <utility>
#include <uv.h>

#include <network/Connection.h>

#include "utils/membuf.h"

namespace Server {
namespace Network {

    // Translate c stype callback into class
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
    conn_id_seq(0), state(State::kStopped), task_limit(task_limit), slowRequestMillisecondTime(slow_req_ms_time), is_read_suspend(false),
    is_write_suspend(false) {}

    Worker::~Worker() = default;

    void noop(uv_signal_t *handle, int signum) {}

    void Worker::Start(const struct sockaddr_storage &address) {
        spdlog::info("Start worker {}", this->id);

        // Init loop
        int rc = uv_loop_init(&uvLoop);
        if (rc != 0) {
            spdlog::critical("Failed to create loop: [{}({})]: {}]\n", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to init uv loop");
        }
        uvLoop.data = this;

        // Init signals
        rc = uv_async_init(&uvLoop, &uvStopAsync, delegate<Worker>::callback<&Worker::OnStop>);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_async_init: [{}({})]: {}]\n", uv_err_name(rc), rc,
                                        uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_async_init");
        }
        uvStopAsync.data = this;

        rc = uv_signal_init(&uvLoop, &uvSigPipe);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_signal_init: [{}({})]: {}]\n", uv_err_name(rc), rc,
                                        uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_signal_init");
        }
        uv_signal_start(&uvSigPipe, noop, SIGPIPE);

        // Setup Network
        rc = uv_tcp_init_ex(&uvLoop, &uvNetwork, address.ss_family);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_tcp_init_ex: [{}({})]: {}]", uv_err_name(rc), rc,
                                        uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_tcp_init_ex");
        }
        uvNetwork.data = this;

        // Configure network
        int fd;
        rc = uv_fileno((uv_handle_t *)&uvNetwork, &fd);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_fileno: [{}({})]: {}]", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_fileno");
        }

        rc = uv_tcp_keepalive(&uvNetwork, 1, 60);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_tcp_keepalive: [{}({})]: {}]", uv_err_name(rc), rc,
                                        uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_tcp_keepalive");
        }

        int on = 1;
        rc = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
        if (rc != 0) {
            spdlog::critical("Failed to call setsockopt: [{}({})]: {}]", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call setsockopt");
        }

        rc = uv_tcp_bind(&uvNetwork, (const struct sockaddr *)&address, 0);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_tcp_bind: [{}({})]: {}]", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_tcp_bind");
        }

        rc = uv_listen((uv_stream_t *)&uvNetwork, 511,
                       delegate<Worker, int>::callback<&Worker::OnConnectionOpen>);
        if (rc != 0) {
            spdlog::critical("Failed to call uv_listen: [{}({})]: {}]", uv_err_name(rc), rc, uv_strerror(rc));
            throw std::runtime_error("Failed to call uv_listen");
        }

        // Start thread
        state = State::kRun;
        openHandlersCount = 3; // uvStopAsync, uvSigPipe, uvNetwork
        rc = uv_thread_create(&thread, delegate<Worker>::callback<&Worker::OnRun>, static_cast<void *>(this));
        if (rc != 0) {
            spdlog::critical("Failed to call uv_thread_create: [{}({})]: {}]", uv_err_name(rc), rc,
                                        uv_strerror(rc));
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
        spdlog::info("Worker {} stopping", id);
        if (uv_is_closing((uv_handle_t *)&uvStopAsync) == 0) {
            uv_close((uv_handle_t *)&uvStopAsync, delegate<Worker>::callback<&Worker::OnHandleClosed>);
        }

        ToStopping();
    }

    void Worker::OnAllocate(uv_handle_t *conn, size_t suggested_size, uv_buf_t *buf) {
        assert(conn);
        assert(buf);

        spdlog::trace("On allocate");

        auto newBuf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
        buf->base = newBuf.base;
        buf->len = newBuf.len;
    }

    void echo_write(uv_write_t *req, int status) {
        if (status == -1) {
            fprintf(stderr, "Write error!\n");
        }
        char *base = (char*) req->data;
        free(base);
        free(req);
    }

    void Worker::OnRead(uv_stream_t *conn, ssize_t nread, const uv_buf_t *buf) {
        assert(conn);
        spdlog::info("OnRead");

        Connection *pconn = (Connection *)(conn);
        spdlog::info("Data avalible: {}", nread);

        if (nread < 0) {
            spdlog::info("Closing connection on OnRead");
            TryCloseConnection(pconn);
            return;
        }

        free(buf->base);
    }

    void Worker::OnConnectionClose(uv_handle_t *) {
        spdlog::info("Trying to close connection");
    }

    void Worker::OnConnectionOpen(uv_stream_t *server, int status) {
        assert(server);
        if (state == State::kStopping) {
            spdlog::debug("New connection block, state stoppong: {}", conn_id_seq + 1);
            return;
        }
        if (status != 0) {
            spdlog::debug("Failed to open connection: {} because {]", conn_id_seq + 1, uv_strerror(status));
            return;
        }

        auto pconn = new Connection();
        spdlog::info("New connection: {}", conn_id_seq + 1);
        pconn->id = ++conn_id_seq;
        pconn->_state = details::State::Open;
        pconn->handler.data = this;
        pconn->write_async.data = this;
        pconn->is_open = true;
        // Increment open handlers counter
        openHandlersCount++;

        // Init connection
        int rc = uv_tcp_init(&uvLoop, (uv_tcp_t *)pconn);
        assert(rc == 0); // impossible

        // Increment open handlers counter
        openHandlersCount++;

        // Setup client socket
        rc = uv_accept(server, (uv_stream_t *)pconn);
        if (rc != 0) {
            spdlog::error("Failed to call uv_accept: [{}({})]: {}]", uv_err_name(rc), rc,
                                             uv_strerror(rc));
            uv_close((uv_handle_t *)(pconn), delegate<Worker>::callback<&Worker::OnConnectionClose>);
            return;
        }

        // if suspend on read and write then not open to read data
        if (!is_write_suspend || !is_read_suspend) {
            // Connection starts by reading new data from it
            rc = uv_read_start((uv_stream_t *)pconn,
                               delegate<Worker, size_t, uv_buf_t *>::callback<&Worker::OnAllocate>,
                               delegate<Worker, ssize_t, const uv_buf_t *>::callback<&Worker::OnRead>);
            if (rc != 0) {
                spdlog::critical("Failed to call uv_read_start: [{}({})]: {}]", uv_err_name(rc), rc,
                                                    uv_strerror(rc));
                uv_close((uv_handle_t *)(pconn), delegate<Worker>::callback<&Worker::OnConnectionClose>);
                return;
            }
        } else {
            spdlog::debug("Connection suspend on accept");
        }
    }

    void Worker::OnHandleClosed(uv_handle_t *) {
        spdlog::info("Handler closed");
        openHandlersCount--;
        StoppedIfRequestedAndPossible();
    }

    void Worker::TryCloseConnection(Server::Network::Connection *pconn) {
         spdlog::info("Try to close connection\n");
        assert(pconn);
        spdlog::debug("Try to close connection: state={}", pconn->_state);

        // Deffer close callback from multiple calls as we dfinetely don't want to delete memory twice!
        assert(!uv_is_closing((uv_handle_t *)&pconn->write_async));
        uv_close((uv_handle_t *)&pconn->write_async,
                 delegate<Worker>::callback<&Worker::OnWriteAsyncClosed>);
        pconn->_state = details::State::WatcherClosing;
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

    void Worker::StoppedIfRequestedAndPossible() {
        if (state == State::kStopped && openHandlersCount == 1) {
            uv_signal_stop(&uvSigPipe);
            openHandlersCount--;
        } else if (state == State::kStopped && openHandlersCount == 0) {
            int rc = uv_loop_close(&uvLoop);
            if (rc != 0) {
                spdlog::critical("Failed to call uv_loop_close: [{}({})]: {}\n",  uv_err_name(rc), rc,
                                 uv_strerror(rc));
            }
            state = State::kStopped;
        }
    }

    void Worker::OnWriteAsyncClosed(uv_handle_t *handle) {
        assert(handle);

        Connection *pconn = reinterpret_cast<Connection *>((char *)handle - offsetof(Connection, write_async));
        assert((uv_handle_t *)&pconn->write_async == handle);
        assert(pconn->_state == details::State::WatcherClosing);
        spdlog::info("Closing connection watcher");

        // openHandlersCount--;
        pconn->_state = details::State::WatcherClosed;
        TryCloseConnection(pconn);
    }

    void Worker::ParseAndExecute(Server::Network::Connection *pconn) {

    }


} // Server
} // Network