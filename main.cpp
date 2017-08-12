#include <cstdlib>

#include <iostream>
#include <fstream>

#include "bulsat_api.h"
#include "playlist_server.h"

#include <event2/event.h>

using namespace std;

void print_usage(const char *name) {
    std::cerr << name << " <user> <pass>" << std::endl;
}

class bsc_handler {
public:

    bsc_handler(bulsat_api& bsc_api, const std::string& playlist, bool login)
        : bulsat_api_(bsc_api)
        , playlist_(playlist)
        , login_(login) {

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
            std::clog << "BSC HANDLER: on_login_result: logged in" << std::endl;
            if(login_) {
                bulsat_api_.request_channel_list();
            } else {
                bulsat_api_.logout();
            }
        } else {
            std::clog << "BSC HANDLER: on_login_result: failed to login" << std::endl;
        }
    }

    static void on_channels_result(int res_code, std::vector<response_channel*>* playlist/* transfer ownership*/, void* obj) {
        bsc_handler* handler = static_cast<bsc_handler*>(obj);
        handler->channel_list_result_handler(res_code, playlist);
    }

    void channel_list_result_handler(int code, std::vector<response_channel*> *channels) {

        if(code != 200) {
            std::clog << "BSC HANDLER: channel_list_result_handler: failed to reqeust channels list" << std::endl;
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

        std::clog << "BSC HANDLER: channel_list_result_handler: playlist generated and stored in " << playlist_ << std::endl;

    }

    static void on_logout_result(int res_code, const response_authentication& res, void* obj) {
        bsc_handler* handler = static_cast<bsc_handler*>(obj);
        handler->logout_result_handler(res_code, res);
    }

    void logout_result_handler(int res_code, const response_authentication& res) {
        if(res_code == 200 && res.logged == "false") {
            std::clog << "BSC HANDLER: logout_result_handler: logged out" << std::endl;
        } else {
            std::clog << "BSC HANDLER: logout_result_handler: failed to logout" << std::endl;
        }
    }

public:

    void login_execute() {
        bulsat_api_.login();
    }

    void logout_execute() {
        bulsat_api_.login();
    }

private:
    bulsat_api& bulsat_api_;
    const std::string playlist_;
    bool login_;
};

int main(int argc, char *argv[]) {

    bool login = false;

    if(argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string bsc_user = argv[1];
    std::string bsc_pass = argv[2];

    event_base* base = event_base_new();

    playlist_server server(base, bsc_user, bsc_pass);

    if(!server.initialize()) {
        return 1;
    }

    std::clog << " Working ... " << std::endl;

    event_base_dispatch(base);
    event_base_free(base);

    return 0;
}
