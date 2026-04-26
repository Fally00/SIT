cmake_minimum_required(VERSION 3.16)
project(SIT
    VERSION     1.3.0
    DESCRIPTION "System Insight Toolkit — lightweight cross-platform system monitor"
    LANGUAGES   CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Source files ──────────────────────────────────────────────────
set(SIT_SOURCES
    src/main.cpp
    src/core/systemInfo.cpp
    src/core/health.cpp
    src/platform/platform.cpp
    src/cli/cli.cpp
    src/formatter/formatter.cpp
    src/integrity/integrity.cpp
)

add_executable(sysinfo ${SIT_SOURCES})

# All project headers are referenced relative to src/
target_include_directories(sysinfo PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ── Platform-specific linking ─────────────────────────────────────
if(WIN32)
    target_link_libraries(sysinfo PRIVATE advapi32)
endif()

# ── Compiler warnings ────────────────────────────────────────────
if(MSVC)
    target_compile_options(sysinfo PRIVATE /W4)
else()
    target_compile_options(sysinfo PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Install ──────────────────────────────────────────────────────
install(TARGETS sysinfo RUNTIME DESTINATION bin)
