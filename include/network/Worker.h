#ifndef SERVER_WORKER_H
#define SERVER_WORKER_H

#include <uv.h>

#include "Connection.h"

namespace Server {
namespace Network {

class Worker {
private:

    // Current worker state
    enum class State {
        kRun,
        kStopping, // ctrl-c
        kStopped // done
    };

    bool is_write_suspend;
    bool is_read_suspend;

    // TODO add suspend commands

    /*
     * Work passed to the worker thread pool and back in order to execute
     * some command
     */
    struct Task {
        // Async signal to be called once task execution is complete
        uv_async_t done;

        // Type of command that should be executed by this task. It could be that command
        // was't read properly, so we can't relay on the command field
        uint32_t type;

        // NUmber of command in the connection sequence
        uint32_t sync;

        // Connection that executes that command
        Connection *connection;
    };


    // Current worker id
    uint32_t id;

    // Worker's current operation state
    State state;

    // Thread running current worker
    uv_thread_t thread;

    // Loops used to process network event. All workers of the same server shares a single port
    // and use SO_SHAREDPROTO to kernel distribute the load
    uv_loop_t uvLoop;

    // Signal to stop worker
    uv_async_t uvStopAsync;

    // Loop handler for sigpipe signal
    uv_signal_t uvSigPipe;

    // TCP/IP socket user by server to listen incoming connection
    uv_tcp_t uvNetwork;

    // Connection unique id generator, use simple sequence strategy
    uint32_t conn_id_seq;

    // Number of open uv handlers: async, sockets, connections, e. t. c.
    uint32_t openHandlersCount;

    uint32_t task_limit;
    uint32_t slowRequestMillisecondTime;

protected:

    // Called by threads once started
    void OnRun();

    // Called once external thread request worker stop
    void OnStop(uv_async_t *);

    // Called once some handler is complete
    void OnHandleClosed(uv_handle_t *);

    // Called by UV once new connection arrived
    void OnConnectionOpen(uv_stream_t *, int);

    // Called by UV when connection gets closed by some reason
    void OnConnectionClose(uv_handle_t *);

    // LibUV used that method just before call OnRead to allocate temporary buffer for the input
    void OnAllocate(uv_handle_t *, size_t suggested_size, uv_buf_t *buf);

    // Connection is ready to read data
    void OnRead(uv_stream_t *, ssize_t nread, const uv_buf_t *buf);

    // Connection notified by some other thread that some data to be send appears in the queue,
    // it should wakeup, enable write side of connection and start serialize output queue
    void OnWriteAsync(uv_async_t *handle);

    // Called once libuv release all internal resources associated with pconn->write_async handler and memory
    // allocated for the task could be reclaimed
    void OnWriteAsyncClosed(uv_handle_t *handle);

    void ParseAndExecute(Connection *pconn);

    void OnStopping(uv_handle_t *, void *);

    void TryCloseConnection(Connection *pconn);

    void ToStopping();
    void StoppedIfRequestedAndPossible();

public:
    Worker(uint32_t id, uint32_t task_limit, uint32_t slow_req_ms_time);
    ~Worker();

    // Starts worker thread
    // Once method return worker is ready to accept and process new connections and commands
    void Start(const struct sockaddr_storage &);

    // Stop current worker execution
    void Stop();
};

} // namespace Network
} // namespace Server

#endif //SERVER_WORKER_H
