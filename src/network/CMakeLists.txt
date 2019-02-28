set(SOURCE_FILES
        Server.cpp
        )

add_library(Network ${SOURCE_FILES})
target_link_libraries(Network uv spdlog)