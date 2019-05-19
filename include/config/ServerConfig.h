#ifndef SERVER_SERVERCONFIG_H
#define SERVER_SERVERCONFIG_H

#include <cstddef>
#include <cstdint>
#include <thread>

namespace Server {

class ServerConfig {
public:
    int Port = 80;

    size_t Workers =  std::thread::hardware_concurrency();

    size_t MemoryLimit = 10 * 1024 * 1024L;

    size_t WorkerTaskLimit = 500;

    size_t SlowRequestMillisecondTime = 1000;

    ServerConfig(){};

    ServerConfig(int port, int thr, size_t mem_limit, size_t task_limit, uint32_t slow_req_time) :
            Port(port), Workers(thr), MemoryLimit(mem_limit), WorkerTaskLimit(task_limit),
            SlowRequestMillisecondTime(slow_req_time) {}
};

} // namespace Server

#endif //SERVER_SERVERCONFIG_H
