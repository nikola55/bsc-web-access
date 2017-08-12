project(bulsat_api)
cmake_minimum_required(VERSION 2.8)

set(SRC_LIST
    main.cpp
    auth.cpp
    bulsat_api.cpp
    crypto.cpp
    form_data.cpp
    http_request.cpp
    playlist_server.cpp
    structs.cpp
)

# add_definitions(-std=c++03)

include_directories("${PROJECT_SOURCE_DIR}/jsoncpp/include")
add_subdirectory(jsoncpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} jsoncpp ssl crypto curl event_core event_extra pthread)
