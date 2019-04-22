set(SOURCE_FILES
        Server.cpp
        Worker.cpp
        http/HTTPUtility.cpp
        )

set(CMAKE_CXX_STANDARD 17)

add_library(Network ${SOURCE_FILES})
target_link_libraries(Network stdc++fs pthread uv spdlog)