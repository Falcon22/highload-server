include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${LIBEVENT_INCLUDE_DIRS})

add_subdirectory(config)
add_subdirectory(network)

set(CMAKE_CXX_STANDARD 17)

## build service
set(SERVER_SOURCE_FILES main.cpp)
add_executable(server ${SERVER_SOURCE_FILES})
target_link_libraries(server Config Network stdc++fs pthread)
