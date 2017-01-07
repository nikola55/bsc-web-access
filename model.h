#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <ostream>


struct Program {
    std::wstring start_ts;
    std::wstring stop_ts;
    std::wstring title;
    std::wstring desc;
};

struct Channel  {
    std::wstring channel;
    std::wstring epg_name;
    std::wstring title;
    std::wstring genre;
    std::wstring sources;
    Program *program;
};

struct AuthResponse {
    std::wstring logged;
};

std::ostream& operator<<(std::ostream &os, const Channel &ch);

#endif // MODEL_H
