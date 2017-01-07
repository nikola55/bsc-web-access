#ifndef AUTH_H
#define AUTH_H
#include <string>
#include <json/json.h>
#include "http_request.h"

typedef void (*c_auth_success)(int);

class Auth {

    Auth(const Auth&a)  { }

    Auth& operator=(const Auth&) { }

public:

    Auth(std::string cf) :
        cookie_file(cf),
        auth_callback(0),
        logout_callback(0) {

    }

    void authenticate(std::string user, std::string pass);

    void logout();

    void onAuthentication(c_auth_success ac) {
        auth_callback = ac;
    }

    void onLogout(c_auth_success ac) {
        logout_callback = ac;
    }

protected:

    bool requestSessionKey();

    bool doLogin(std::string user, std::string pass);

    bool doLogout();

private:
    Json::Value val;
    std::string cookie_file;
    std::string key;
    std::string session;
    c_auth_success auth_callback;
    c_auth_success logout_callback;

};

#endif // AUTH_H
