#include "model.h"

std::wostream& operator<<(std::wostream &os, const Channel &ch) {
    os << "channel: {" <<std::endl
       << "    channel: '" << ch.channel << "'" << std::endl
       << "    epg_name: '" << ch.epg_name << "'" << std::endl
       << "    title: '" << ch.title << "'" << std::endl
       << "    genre: '" << ch.genre << "'" << std::endl
       << "    sources: '" << ch.sources << "'" << std::endl
       << "    program: {" << std::endl
       << "        start_ts: '" << ch.program->start_ts << "'" << std::endl
       << "        stop_ts: '" << ch.program->stop_ts << "'" << std::endl
       << "        title: '" << ch.program->title << "'" << std::endl
       << "        desc: '" << ch.program->desc << "'" << std::endl
       << "    }" << std::endl
       << "}";
    return os;
}


