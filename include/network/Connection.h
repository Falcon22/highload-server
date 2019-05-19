#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "uv.h"

namespace Server {
namespace Network {

namespace details {
enum State { Open, WatcherClosing, WatcherClosed, SocketClosing, SocketClosed };
};

/*
 * Structure carries state of a single connection
 */
struct Connection {
    // Extends UV stream to pass connection around through libuv
    // internal structures
    uv_stream_t handler;

    // Async signal to be called once are some data to be send
    // in the connection
    uv_async_t write_async;

    // Connection state
    details::State _state;

    // Connection unique number
    uint32_t id;

    // Input buffer
    char* in;

    // True if header was parsed from the input buffer
    bool header;

    bool is_open;
};

} // namespace Network
} // namespace Server

#endif //SERVER_CONNECTION_H
