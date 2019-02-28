#include <config/Config.h>

#include <memory>
#include <config/ServerConfig.h>

namespace Server {

Config::Config() {
    serverConfig = std::make_shared<ServerConfig>();
}

Config::~Config() {}

void Config::Load(const std::string file) {
}

}
