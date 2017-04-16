#include "bulsat_api.h"
#include <json/json.h>
#include <cassert>
#include <iostream>

#include "form_data.h"
#include "crypto.h"

bulsat_api::bulsat_api(const std::string &cookiefile, const std::string &user, const std::string &pass)
    : user_(user)
    , pass_(pass)
    , os_id_("pcweb")
    , user_agent_("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36")
    , base_url_("https://api.iptv.bulsat.com")
    , auth_url_(base_url_ + (os_id_=="pcweb"?"/auth":"?auth"))
    , channel_url_("https://api.iptv.bulsat.com/tv/"+os_id_+"/live")
    , session_state_(SESSION_NOT_LOGGED_IN)
    , cookie_file_(cookiefile) {

}

bulsat_api::~bulsat_api() {

}

void bulsat_api::login() {
    if(session_state_ != SESSION_NOT_LOGGED_IN) {

        assert("Requested login in incorrect state" == 0);
        return;
    }

    const std::string user = user_;
    const std::string pass = pass_;

    { // check if we are logged in
        http_request http;
        http.request_header("Origin","https://test.iptv.bulsat.com");
        http.request_header("Referer","https://test.iptv.bulsat.com/iptv-login.php");
        http.request_header("User-Agent", user_agent_);
        int res = http.post(auth_url_, reinterpret_cast<const uint8_t*>(""), 0);
        if(res != 200) {
            if(on_login_result_cb_)
                on_login_result_cb_(res, response_authentication(), on_login_result_ctx_);
            return;
        }
        session_ = http.response_header("SSBULSATAPI");
        session_key_ = http.response_header("CHALLENGE");

        Json::Reader reader;
        Json::Value login_response;
        const char *resp = reinterpret_cast<const char*>(http.response());
        if(reader.parse(&resp[0], &resp[http.response_len()], login_response)) {
            std::clog << "BULSATAPI get session keys: " << login_response["Logged"].asString() << std::endl;
            if(login_response["Logged"].asString() == "true") {
                session_state_ = SESSION_LOGGED_IN;
                auth_response_.logged = login_response["Logged"].asString() ;
                if(on_login_result_cb_)
                    on_login_result_cb_(200, auth_response_, on_login_result_ctx_);
                return;
            }
        }
    }
    { // if we got here we are not logged in

        http_request http;
        form_data form;

        std::vector<uint8_t> cipher( (pass.size()/16 + 1)*16 );
        AES128_ECB_Enc( reinterpret_cast<const uint8_t*>(session_key_.c_str()), 32,
                        reinterpret_cast<const uint8_t*>(pass.c_str()), pass.size(),
                        &cipher[0], cipher.size());
        char *base64str = base64(&cipher[0], cipher.size());
        std::string enc_pass = base64str;
        delete[] base64str;

        form.add("user", user);
        form.add("pass", enc_pass);
        form.add("device_id", os_id_);
        form.add("device_name",os_id_);
        form.add("os_version",os_id_);
        form.add("os_type", os_id_);
        form.add("app_version", "0.01");
        std::string form_string = form.string();

        std::string content_type = "multipart/form-data; boundary="+form.boundrary().substr(2);
        http.request_header("Content-Type", content_type);
        http.request_header("SSBULSATAPI", session_);
        http.request_header("Origin","https://test.iptv.bulsat.com");
        http.request_header("Referer","https://test.iptv.bulsat.com/iptv-login.php");
        http.request_header("User-Agent", user_agent_);

        int res = http.post(auth_url_, reinterpret_cast<const uint8_t*>(form_string.c_str()), form_string.length());
        if(res != 200) {
            if(on_login_result_cb_)
                on_login_result_cb_(res, response_authentication(), on_login_result_ctx_);
        } else {
            Json::Reader reader;
            Json::Value login_response;
            const char *resp = reinterpret_cast<const char*>(http.response());
            if(reader.parse(&resp[0], &resp[http.response_len()], login_response)) {
                std::clog << "BULSATAPI get session keys: " << login_response["Logged"].asString() << std::endl;
                auth_response_.logged = login_response["Logged"].asString() ;
                if(login_response["Logged"].asString() == "true") {
                    session_state_ = SESSION_LOGGED_IN;
                }
                if(on_login_result_cb_)
                    on_login_result_cb_(200, auth_response_, on_login_result_ctx_);
                return;
            } else {
                if(on_login_result_cb_)
                    on_login_result_cb_(200, response_authentication(), on_login_result_ctx_);
            }
        }
    }
}

void bulsat_api::request_channel_list() {
    if( session_state_ != SESSION_LOGGED_IN) {
        assert("Requested logout in incorrect state" == 0);
        return;
    }

    {
        http_request http;
        http.request_header("SSBULSATAPI", session_);
        http.request_header("Origin","https://test.iptv.bulsat.com");
        http.request_header("Referer","https://test.iptv.bulsat.com/iptv-login.php");
        http.request_header("User-Agent", user_agent_);
        int res = http.post(channel_url_, reinterpret_cast<const uint8_t*>(""), 0);
        if(res != 200) {
            if(on_channel_list_result_cb_)
                on_channel_list_result_cb_(res, std::list<response_channel>(), on_channel_list_result_ctx_);
        } else {
            Json::Reader reader;
            Json::Value channel_response;
            const char *resp = reinterpret_cast<const char*>(http.response());
            if(reader.parse(&resp[0], &resp[http.response_len()], channel_response)) {
                std::clog << "BULSATAPI channel list size: " << channel_response.size() << std::endl;
                channel_list_.resize(channel_response.size());
                std::list<response_channel>::iterator ch_iter = channel_list_.begin();
                for(size_t i = 0 ; i < channel_response.size() ; i++) {
                    assert(ch_iter != channel_list_.end());
                    response_channel& curr_channel = *ch_iter++;
                    Json::Value &channel = channel_response[int(i)];
                    if(channel.isObject()) {
                        curr_channel.channel = channel["channel"].asString();
                        curr_channel.epg_name = channel["epg_name"].asString();
                        curr_channel.title = channel["title"].asString();
                        curr_channel.genre = channel["genre"].asString();
                        curr_channel.radio = channel["radio"].asBool();
                        curr_channel.sources = channel["sources"].asString();
                        Json::Value& program = channel["program"];
                        if(program.isObject()) {
                            curr_channel.program.start_ts = program["startts"].asString();
                            curr_channel.program.stop_ts = program["stopts"].asString();
                            curr_channel.program.title = program["title"].asString();
                            curr_channel.program.desc = program["desc"].asString();
                        }
                    }
                }
                session_state_ = SESSION_CHANNEL_LIST_READY;
                if(on_channel_list_result_cb_)
                    on_channel_list_result_cb_(200, channel_list_, on_channel_list_result_ctx_);
            }
        }
    }
}

void bulsat_api::logout() {
    if( session_state_ != SESSION_LOGGED_IN && session_state_ != SESSION_CHANNEL_LIST_READY) {
        assert("Requested logout in incorrect state" == 0);
        return;
    }

    {
        http_request http;
        form_data form;
        form.add("logout", "1");
        std::string content_type = "multipart/form-data; boundary="+form.boundrary().substr(2);
        std::string form_string = form.string();
        http.request_header("Content-Type", content_type);
        http.request_header("SSBULSATAPI", session_);
        http.request_header("User-Agent", user_agent_);
        int res = http.post(auth_url_, reinterpret_cast<const uint8_t*>(form_string.c_str()), form_string.length());
        if(res != 200) {
            if(on_login_result_cb_)
                on_login_result_cb_(res, response_authentication(), on_login_result_ctx_);
        } else {
            Json::Reader reader;
            Json::Value logout_response;
            const char *resp = reinterpret_cast<const char*>(http.response());
            if(reader.parse(&resp[0], &resp[http.response_len()], logout_response)) {
                std::clog << "BULSATAPI logout response: " << logout_response["Logged"].asString() << std::endl;
                auth_response_.logged = logout_response["Logged"].asString() ;
                if(logout_response["Logged"].asString() == "true") {
                    session_state_ = SESSION_NOT_LOGGED_IN;
                }
                if(on_login_result_cb_)
                    on_login_result_cb_(200, auth_response_, on_login_result_ctx_);
            } else {
                if(on_login_result_cb_)
                    on_login_result_cb_(200, response_authentication(), on_login_result_ctx_);
            }
        }
    }
}

void bulsat_api::set_on_login_result(bulsat_api::on_login_result on_login_res_cb, void *ctx) {
    on_login_result_cb_ = on_login_res_cb;
    on_login_result_ctx_ = ctx;
}

void bulsat_api::set_on_channel_list_result(bulsat_api::on_channel_list_result on_channel_list_res_cb, void *ctx) {
    on_channel_list_result_cb_ = on_channel_list_res_cb;
    on_channel_list_result_ctx_ = ctx;
}
