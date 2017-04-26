#ifndef BULSAT_API_H
#define BULSAT_API_H

#include <string>
#include <vector>

#include "http_request.h"

#include "structs.h"

class bulsat_api {
public:
    bulsat_api(const std::string& cookiefile, const std::string& user, const std::string& pass);
    ~bulsat_api();
private:
    bulsat_api(const bulsat_api&);
    bulsat_api& operator=(const bulsat_api&);
private:
    enum session_state {
        SESSION_NOT_LOGGED_IN,
        SESSION_LOGGED_IN,
        SESSION_CHANNEL_LIST_READY
    };
public:
    void login();
    void request_channel_list();
    void logout();
public:
    typedef void(*on_login_result)(int, const response_authentication&, void*);
    typedef void(*on_channel_list_result)(int, std::vector<response_channel*>* /* transfer ownership*/, void*);

    void set_on_login_result(on_login_result, void*);
    void set_on_channel_list_result(on_channel_list_result, void*);
    void set_on_logout_result(on_login_result, void*);

private:
    std::string user_;
    std::string pass_;
    std::string os_id_;
    std::string user_agent_;
    std::string base_url_;
    std::string auth_url_;
    std::string channel_url_;
    session_state session_state_;
    response_authentication auth_response_;
    std::string cookie_file_;
    std::string session_;
    std::string session_key_;
    on_login_result on_login_result_cb_;
    void* on_login_result_ctx_;
    on_channel_list_result on_channel_list_result_cb_;
    void* on_channel_list_result_ctx_;
    on_login_result on_logout_result_cb_;
    void* on_logout_result_ctx_;
};

#endif // BULSAT_API_H
