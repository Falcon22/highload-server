#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

namespace Server {
namespace Network {

/*
 * Structure carries state of a single connection
 */
struct Conncetion {
    // Extends UV stream to pass connection around through libuv
    // internal structures
    uv_stream_t handler;

    // Async signal to be called once are some data to be send
    // in the connection
    uv_async_t write_async;

    // Connection unique number
    uint32_t id;

    bool is_open;
};

} // namespace Network
} // namespace Server

#endif //SERVER_CONNECTION_H
