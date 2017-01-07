project(bulsat_api)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

add_definitions(-std=c++98)

include_directories("${PROJECT_SOURCE_DIR}/jsoncpp")
add_subdirectory(jsoncpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} crypto ssl curl jsoncpp)
