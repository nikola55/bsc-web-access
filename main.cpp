#include <cstdlib>

#include <iostream>
#include <fstream>

#include "bulsat_api.h"

using namespace std;

void print_usage_and_exit(const char *name) {
    std::cerr << name << " (logout)|(login <user> <pass> <playlist>) " << std::endl;
    exit(1);
}

class bsc_handler {
public:

    bsc_handler(bulsat_api& bsc_api, const std::string& playlist)
        : bulsat_api_(bsc_api)
        , playlist_(playlist) {

        bulsat_api_.set_on_login_result(&bsc_handler::on_login_result, this);
        bulsat_api_.set_on_channel_list_result(&bsc_handler::on_channels_result, this);
        bulsat_api_.set_on_logout_result(&bsc_handler::on_logout_result, this);

    }

    ~bsc_handler() {}

private:
    bsc_handler(const bsc_handler&);
    void operator=(const bsc_handler&);
private:

    static void on_login_result(int res_code, const response_authentication& res, void* obj) {
        bsc_handler* handler = static_cast<bsc_handler*>(obj);
        handler->login_result_handler(res_code, res);
    }

    void login_result_handler(int code, const response_authentication& res) {
        if(code == 200 && res.logged == "true") {
            bulsat_api_.request_channel_list();
        } else {
            std::clog << "failed to login" << std::endl;
        }
    }

    static void on_channels_result(int res_code, std::vector<response_channel*>* playlist/* transfer ownership*/, void* obj) {
        bsc_handler* handler = static_cast<bsc_handler*>(obj);
        handler->channel_list_result_handler(res_code, playlist);
    }

    void channel_list_result_handler(int code, std::vector<response_channel*> *channels) {

        if(code != 200) {
            std::clog << "failed to reqeust channels list" << std::endl;
            return;
        }

        std::ofstream playlist_file(playlist_.c_str());

        if(playlist_file.is_open()) {

            std::ostream &out = playlist_file;
            out << "#EXTM3U" << std::endl;


            std::vector<response_channel*>::iterator it = channels->begin();
            std::vector<response_channel*>::iterator end = channels->end();

            for(; it != end ; it++) {
                response_channel* chann = *it;
                out << "#EXTINF:-1 group-title=\"" << chann->genre
                    << "\" tvg-id=\"" << chann->epg_name
                    << "\","<< chann->title << std::endl;
                out << chann->sources << std::endl;
            }

            playlist_file.close();
        }

        for(; channels->size() != 0 ; channels->pop_back())
            delete channels->back();
        delete channels;

        std::clog << "playlist generated and stored in " << playlist_ << std::endl;

    }

    static void on_logout_result(int res_code, const response_authentication& res, void* obj) {
        bsc_handler* handler = static_cast<bsc_handler*>(obj);
        handler->logout_result_handler(res_code, res);
    }

    void logout_result_handler(int res_code, const response_authentication& res) {
        if(res_code == 200 && res.logged == "false") {
            std::clog << "logged out" << std::endl;
        } else {
            std::clog << "failed to logout" << std::endl;
        }
    }

public:

    void login_execute() {
        bulsat_api_.login();
    }

    void logout_execute() {
        bulsat_api_.logout();
    }

private:
    bulsat_api& bulsat_api_;
    const std::string playlist_;
};

int main(int argc, char *argv[]) {

    bool login = false;

    if(argc < 2) {
        print_usage_and_exit(argv[0]);
    } else {
        std::string cmd = argv[1];
        if(cmd == "login") {
            login = true;
        } else if (cmd == "logout") {
            login = false;
        } else {
            print_usage_and_exit(argv[0]);
        }
    }

    std::string user;
    std::string pass;
    std::string playlist;

    if(login) {

        if(argc != 5)
            print_usage_and_exit(argv[0]);

        user = argv[2];
        pass = argv[3];
        playlist = argv[4];

    } else {
        if(argc != 2)
            print_usage_and_exit(argv[0]);
    }

    bulsat_api bsc_api("cookies.txt", user, pass);

    bsc_handler handler(bsc_api, playlist);

    if(login) {
        handler.login_execute();
    } else {
        handler.logout_execute();
    }

    return 0;
}
