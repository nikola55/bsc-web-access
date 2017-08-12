#include "playlist_server.h"

#include <sstream>
#include <iostream>
#include <cassert>
#include <cstring>

#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/event.h>

playlist_server::playlist_server(event_base* base, 
                                const std::string& bsc_user, 
                                const std::string& bsc_pass, 
                                int port, 
                                const std::string& path,
                                unsigned refresh_rate_sec
                                )
    : base_(base)
    , http_(NULL)
    , handle_(NULL)
    , refresh_timer_(event_new(base_, -1, EV_PERSIST, &playlist_server::on_refresh_playlist, this))
    , path_(path)
    , api_("", bsc_user, bsc_pass)
    , logged_in_(false)
    , have_channels_(false)
    , updating_(false)
    , update_thread_(nullptr) {

    api_.set_on_login_result(&playlist_server::on_login_result, this);
    api_.set_on_channel_list_result(&playlist_server::on_channel_list_result, this);

    http_ = evhttp_new(base_);
    assert(NULL != http_);
    handle_ = evhttp_bind_socket_with_handle(http_, "127.0.0.1", port);
    if(NULL == handle_) {
        std::clog << "BSC HANDLER: playlist_server: bind failed " << std::strerror(errno) << std::endl;
    }

    evhttp_set_cb(http_, path.c_str(), &playlist_server::write_playlist, this);
    evhttp_set_gencb(http_, &playlist_server::write_default, this);

    timeval timeout;
    timeout.tv_sec = refresh_rate_sec;
    timeout.tv_usec = 0;
    event_add(refresh_timer_, &timeout);

}

playlist_server::~playlist_server() {
    event_free(refresh_timer_);
    evhttp_free(http_);
}

bool playlist_server::initialize() {
    api_.login();
    return logged_in_ && have_channels_;
}

void playlist_server::on_login_result(int code, const response_authentication& res, void* arg) {
    playlist_server* ctx = static_cast<playlist_server*>(arg);
    ctx->handle_on_login_result(code, res);
}

void playlist_server::handle_on_login_result(int code, const response_authentication& res) {
    if(code == 200 && res.logged == "true") {
        std::clog << "BSC HANDLER: on_login_result: logged in" << std::endl;
        logged_in_ = true;
        api_.request_channel_list();
    } else {
        std::clog << "BSC HANDLER: on_login_result: failed to login" << std::endl;
    }
}

void playlist_server::on_channel_list_result(int code, std::vector<response_channel*>* list /* transfer ownership*/, void* arg) {
    playlist_server* ctx = static_cast<playlist_server*>(arg);
    ctx->handle_on_channel_list_result(code, list);
}

void playlist_server::handle_on_channel_list_result(int code, std::vector<response_channel*>* channels) {
    if(code != 200) {
        std::clog << "BSC HANDLER: channel_list_result_handler: failed to reqeust channels list" << std::endl;
        return;
    }
    std::stringstream playlist_str;
    playlist_str << "#EXTM3U" << std::endl;


    std::vector<response_channel*>::iterator it = channels->begin();
    std::vector<response_channel*>::iterator end = channels->end();

    for(; it != end ; it++) {
        response_channel* chann = *it;
        playlist_str << "#EXTINF:-1 group-title=\"" << chann->genre
            << "\" tvg-id=\"" << chann->epg_name
            << "\","<< chann->title << std::endl;
        playlist_str << chann->sources << std::endl;
    }

    for(; channels->size() != 0 ; channels->pop_back())
            delete channels->back();
    delete channels;

    playlist_mutex_.lock();
    playlist_ = playlist_str.str();
    have_channels_ = true;
    playlist_mutex_.unlock();
    
    std::clog << "BSC HANDLER: channel_list_result_handler: playlist generated" << std::endl;

    // std::cout << playlist_ << std::endl;
}

void playlist_server::write_default(evhttp_request *req, void *arg) {
    playlist_server* ctx = static_cast<playlist_server*>(arg);
    ctx->handle_write_default(req);
}

void playlist_server::handle_write_default(evhttp_request *req) {
    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET:
            break;
        case EVHTTP_REQ_POST: 
        case EVHTTP_REQ_HEAD: 
        case EVHTTP_REQ_PUT: 
        case EVHTTP_REQ_DELETE: 
        case EVHTTP_REQ_OPTIONS: 
        case EVHTTP_REQ_TRACE: 
        case EVHTTP_REQ_CONNECT: 
        case EVHTTP_REQ_PATCH: 
        default: 
            evhttp_send_reply(req, 501, "Not Implemented", NULL);
            return;
    }

    evbuffer *evb = evbuffer_new();

    evbuffer_add_printf(evb, "playlist is available at %s", path_.c_str());

    evhttp_send_reply(req, 200, "OK", evb);

}

void playlist_server::write_playlist(evhttp_request *req, void *arg) {
    playlist_server* ctx = static_cast<playlist_server*>(arg);
    ctx->handle_write_playlist(req);
}

void playlist_server::handle_write_playlist(evhttp_request *req) {
    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET:
            break;
        case EVHTTP_REQ_POST: 
        case EVHTTP_REQ_HEAD: 
        case EVHTTP_REQ_PUT: 
        case EVHTTP_REQ_DELETE: 
        case EVHTTP_REQ_OPTIONS: 
        case EVHTTP_REQ_TRACE: 
        case EVHTTP_REQ_CONNECT: 
        case EVHTTP_REQ_PATCH: 
        default: 
            evhttp_send_reply(req, 501, "Not Implemented", NULL);
            return;
    }

    evbuffer *evb = evbuffer_new();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "video/m3u8");
    
    playlist_mutex_.lock();
    std::string copy = playlist_;
    playlist_mutex_.unlock();

    evbuffer_add(evb, copy.c_str(), copy.size());
    evhttp_send_reply(req, 200, "OK", evb);
}

void playlist_server::on_refresh_playlist(int, short, void *arg) {
    playlist_server* ctx = static_cast<playlist_server*>(arg);
    ctx->handle_on_refresh_playlist();
}

void playlist_server::handle_on_refresh_playlist() {
    if(!updating_.exchange(true)) {
        update_thread_ = new std::thread(&playlist_server::update_task, this);
        update_thread_->detach();
    }
}

void playlist_server::update_task() {
    api_.request_channel_list();
    updating_ = false;
}