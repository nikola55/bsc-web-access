#ifndef PLAYLIST_SERVER_H
#define PLAYLIST_SERVER_H

#include <string>
#include <atomic>
#include <mutex>
#include <thread>

#include "bulsat_api.h"

struct event_base;
struct evhttp;
struct evhttp_request;
struct evhttp_bound_socket;
struct event;

class playlist_server {
public:
    playlist_server(event_base* base, 
                    const std::string& bsc_user, 
                    const std::string& bsc_pass,
                    int port = 1989,
                    const std::string& path = "/bulsat_playlist.m3u8",
                    unsigned refresh_rate_sec = (5) /* 4 hours by default */);
    ~playlist_server();
public:
    bool initialize(); // perform login and playlist update on the main thread
private:
    static void on_login_result(int, const response_authentication&, void*);
    void handle_on_login_result(int, const response_authentication&);

    static void on_channel_list_result(int, std::vector<response_channel*>* /* transfer ownership*/, void*);
    void handle_on_channel_list_result(int, std::vector<response_channel*>* /* transfer ownership*/);
private:
    static void write_playlist(evhttp_request *req, void *arg);
    void handle_write_playlist(evhttp_request *req);

    static void write_default(evhttp_request *req, void *arg);
    void handle_write_default(evhttp_request *req);

    static void on_refresh_playlist(int, short, void *arg);
    void handle_on_refresh_playlist();
private:
    void update_task();
private:
    event_base* base_;
    evhttp* http_;
    evhttp_bound_socket* handle_;
    event* refresh_timer_;
    const std::string path_;
    bulsat_api api_;
    bool logged_in_;
    bool have_channels_;
    std::mutex playlist_mutex_;
    std::string playlist_;
    std::atomic<bool> updating_;
    std::thread* update_thread_;

};

#endif