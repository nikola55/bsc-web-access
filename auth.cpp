#include "auth.h"
#include "crypto.h"
#include "form_data.h"
#include <string.h>
#include <iostream>


void Auth::authenticate(std::string user, std::string pass) {

    if(!requestSessionKey()) {
        if(auth_callback != 0) {
            auth_callback(1);
            return;
        }
    }

    doLogin(user, pass);

}

bool Auth::doLogin(std::string user, std::string pass) {

    form_data fd;

    std::vector<uint8_t> cipher( (pass.size()/16 + 1)*16 );

    std::cout << "buffer size " << cipher.size() << std::endl;

    AES128_ECB_Enc(reinterpret_cast<const uint8_t*>(key.c_str()), 32,
                    reinterpret_cast<const uint8_t*>(pass.c_str()), pass.size(),
                    &cipher[0], cipher.size());

    char *base64str = base64(&cipher[0], cipher.size());

    pass = base64str;

    std::cout << "enc(Pass)=" << pass << std::endl;

    delete base64str;

    std::string os_id = "pcweb";

    fd.add("user", user);
    fd.add("pass", pass);
    fd.add("device_id", os_id);
    fd.add("device_name",os_id);
    fd.add("os_version",os_id);
    fd.add("os_type", os_id);
    fd.add("app_version", "0.01");

    std::string contentType = "multipart/form-data; boundary="+fd.boundrary().substr(2);

    http_request http_client(cookie_file);

    http_client.request_header("Origin", "https://test.iptv.bulsat.com");
    http_client.request_header("Referer", "https://test.iptv.bulsat.com/iptv-login.php");

    http_client.request_header("Content-Type", contentType);
    http_client.request_header("SSBULSATAPI", session);

    std::string body = fd.string();

    std::cout << "* * * * request = " << body << std::endl;

    uint32_t code = http_client.post("https://api.iptv.bulsat.com/auth", reinterpret_cast<const uint8_t*>(body.c_str()), body.size());
    uint32_t respLen = http_client.response_len();
    const char *resp = reinterpret_cast<const char*>(http_client.response());
    std::string response(&resp[0], &resp[respLen]);

    std::cout << "* * * * response= " << response << "'" << std::endl;

    if(code != 200) {
        return false;
    }

    http_request req(cookie_file);
    req.request_header("SSBULSATAPI", session);
    req.request_header("Origin", "https://test.iptv.bulsat.com");
    req.request_header("Referer", "https://test.iptv.bulsat.com/iptv-login.php");

    code = req.post("https://api.iptv.bulsat.com/tv/pcweb/live",
                    reinterpret_cast<const uint8_t*>(""), 0);
    if(code == 200) {

        uint32_t chListLen = req.response_len();
        const char * chList = reinterpret_cast<const char*>(req.response());

        Json::Reader reader;

        if(reader.parse(chList, val)) {
            if(val.type() == Json::arrayValue) {
                for(int i = 0 ; i < val.size() ; i++) {
                    Json::Value &chan = val[i];
                    if(chan.isObject()) {
                        std::cout << chan["quality"] << " " << chan["epg_name"] << " " << chan["sources"] << std::endl;
                    }
                }
            }
        }

//        std::cerr << chList << std::endl;
//        std::vector<Channel> channels;
//        parse_channel_list(chList, channels);
//        std::cout << chList << std::endl;

//        std::string chListStr(&chList[0], &chList[chListLen]);
//        std::cout << "Channel list='" << chListStr << "'" << std::endl;
    }

    return true; // check if response is logged=true
}

bool Auth::requestSessionKey() {

    //'CHALLENGE';
    //'SSBULSATAPI';
    http_request http_client(cookie_file);

    uint32_t respCode = http_client.post("https://api.iptv.bulsat.com/auth", reinterpret_cast<const uint8_t*>(""), 0);
    if(respCode != 200) {
        return false;
    }

    key = http_client.response_header("CHALLENGE");
    if(key.size() != 32) {
            return false;
    }

    std::cout << "\n\n\n * * * key='" << key << "' * * * \n\n\n" << std::endl;

    session = http_client.response_header("SSBULSATAPI");

    std::cout << "\n\n\n * * * session='" << session << "' * * * \n\n\n" << std::endl;

    return true;

}

void Auth::logout() {

    if(!doLogout()) {
        if(logout_callback != 0) {
            logout_callback(1);
        }
    }

    if(logout_callback != 0) {
        logout_callback(0);
    }

}

bool Auth::doLogout() {

    form_data fd;
    fd.add("logout", "1");
    std::string contentType = "multipart/form-data; boundary="+fd.boundrary().substr(2);
    http_request http_client(cookie_file);
    http_client.request_header("Content-Type", contentType);
    http_client.request_header("SSBULSATAPI", session);
    std::string body = fd.string();
    uint32_t code = http_client.post("https://api.iptv.bulsat.com/auth",
                                     reinterpret_cast<const uint8_t*>(body.c_str()), body.size());
    if(code != 200) {
        return false;
    }
    uint32_t respLen = http_client.response_len();
    const char *resp = reinterpret_cast<const char*>(http_client.response());
    std::string response(&resp[0], &resp[respLen]);

    std::cout << "* * * * response= " << response << "'" << std::endl;

    return true;
}
