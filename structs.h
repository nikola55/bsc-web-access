#ifndef STRUCTS_H
#define STRUCTS_H
#include <string>

struct response_program {
    std::string start_ts;
    std::string stop_ts;
    std::string title;
    std::string desc;
};

struct response_channel  {
    std::string channel;
    std::string epg_name;
    std::string title;
    std::string genre;
    bool radio;
    std::string sources;
    response_program program;
};

struct response_authentication {
    std::string logged;
};


#endif // STRUCTS_H
