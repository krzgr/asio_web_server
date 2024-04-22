cmake_minimum_required(VERSION 3.15)

project(AsioWebServer)

set(SRC_FILES
    "src/main.cpp"
    "src/WebServer.hpp"
    "src/WebServer.cpp"
    "src/HTTPConnection.hpp"
    "src/HTTPConnection.cpp"
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/boost/boost_1_77_0")
    target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()

if(UNIX)
    target_include_directories(${PROJECT_NAME} PRIVATE "/mnt/c/Program Files/boost/boost_1_77_0")
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)