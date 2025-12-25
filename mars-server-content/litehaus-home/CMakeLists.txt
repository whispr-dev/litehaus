cmake_minimum_required(VERSION 3.16)
project(litehaus-home VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# SimdJSON - try system package first, then fetch
find_package(simdjson QUIET)
if(NOT simdjson_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v3.6.0
    )
    FetchContent_MakeAvailable(simdjson)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CURL_INCLUDE_DIRS})

# ============================================================================
# LITEHAUS-CORE - Main telemetry collection service
# ============================================================================
add_executable(litehaus-core
    src/core/main.cpp
)

target_link_libraries(litehaus-core
    PRIVATE
    simdjson
    ${CURL_LIBRARIES}
    Threads::Threads
)

# ============================================================================
# LITEHAUS-STREAM - WebSocket/UDP broadcast service
# ============================================================================
add_executable(litehaus-stream
    src/stream/main.cpp
)

target_link_libraries(litehaus-stream
    PRIVATE
    simdjson
    Threads::Threads
)

# ============================================================================
# LITEHAUS-LORA - LoRa gateway supervisor (Phase 3)
# ============================================================================
# add_executable(litehaus-lora
#     src/lora/main.cpp
# )

# ============================================================================
# LITEHAUS-RADIO - SDR radio bridge (Phase 4)
# ============================================================================
# add_executable(litehaus-radio
#     src/radio/main.cpp
# )

# ============================================================================
# Installation
# ============================================================================
install(TARGETS litehaus-core litehaus-stream
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION /etc/litehaus
    FILES_MATCHING PATTERN "*.conf" PATTERN "*.toml" PATTERN "*.json"
)

install(DIRECTORY systemd/
    DESTINATION /etc/systemd/system
    FILES_MATCHING PATTERN "*.service"
)

install(DIRECTORY web/
    DESTINATION /var/www/litehaus
)

# ============================================================================
# Development targets
# ============================================================================
add_custom_target(format
    COMMAND clang-format -i ${CMAKE_SOURCE_DIR}/src/**/*.cpp ${CMAKE_SOURCE_DIR}/include/*.hpp
    COMMENT "Formatting source files..."
)

add_custom_target(run-core
    COMMAND ${CMAKE_BINARY_DIR}/litehaus-core -c ${CMAKE_SOURCE_DIR}/config/core.conf
    DEPENDS litehaus-core
    COMMENT "Running litehaus-core..."
)

add_custom_target(run-stream
    COMMAND ${CMAKE_BINARY_DIR}/litehaus-stream -c ${CMAKE_SOURCE_DIR}/config/stream.conf
    DEPENDS litehaus-stream
    COMMENT "Running litehaus-stream..."
)

# Print configuration summary
message(STATUS "")
message(STATUS "🏰 LITEHAUS HOME MASTER NODE 🏰")
message(STATUS "================================")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  CURL:        ${CURL_LIBRARIES}")
message(STATUS "  SimdJSON:    ${simdjson_FOUND}")
message(STATUS "")
