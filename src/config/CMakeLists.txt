set(SOURCE_FILES
        Config.cpp
        )

add_library(Config ${SOURCE_FILES})
target_link_libraries(Config spdlog)