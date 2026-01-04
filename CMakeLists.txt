cmake_minimum_required(VERSION 3.20)
#3.20 is used because its stable and predictable, and FetchContent works properly
project(local-AI VERSION 0.1.0 LANGUAGES CXX)

#using C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#record every compile command for use by clangd or other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

#llama.cpp downloading and building
set(LLAMA_BUILD_COMMON ON CACHE BOOL "" FORCE)
set(LLAMA_CURL OFF CACHE BOOL "" FORCE)

#enable CUDA
set (GGML CUDA ON CACHE BOOL "" FORCE)

FetchContent_Declare(llama
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(llama)

#download JSON parsing library
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

add_executable(local-ai
    src/main.cpp
    src/llm.cpp
    src/parser.cpp
    src/shell.cpp
)

target_include_directories(local-ai PRIVATE src)
target_link_libraries(local-ai PRIVATE
    llama
    common
    nlohmann_json::nlohmann_json
    readline
)
