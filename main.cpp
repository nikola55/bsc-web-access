#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <json/json.h>

#include "crypto.h"
#include "http_request.h"
#include "form_data.h"
#include "auth.h"
#include "model.h"
#include "bulsat_api.h"

using namespace std;

int main(int argc, char* argv[]) {

    if(argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <username> <pass> <dest_file>" << std::endl;
        return 1;
    }

    const std::string os_id = "pcweb";
    const std::string user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36";
    const std::string base_url = "https://api.iptv.bulsat.com";
    const std::string auth_url = base_url + (os_id=="pcweb"?"/auth":"?auth");
    const std::string channel_url = "https://api.iptv.bulsat.com/tv/"+os_id+"/live";
    const std::string user = argv[1];
    const std::string pass = argv[2];
    const std::string playlist_filepath = argv[3];
    const std::string playlist_filepath_tmp = playlist_filepath+".tmp";

    // ---------------------- Get Session and Key ---------------------------
    std::string session, key;
    http_request session_request;
    session_request.request_header("Origin","https://test.iptv.bulsat.com");
    session_request.request_header("Referer","https://test.iptv.bulsat.com/iptv-login.php");
    session_request.request_header("User-Agent", user_agent);
    uint32_t res = session_request.post(auth_url, reinterpret_cast<const uint8_t*>(""), 0);
    if(res != 200) {
        return 1;
    }
    session = session_request.response_header("SSBULSATAPI");
    key = session_request.response_header("CHALLENGE");
    const char *resp = reinterpret_cast<const char*>(session_request.response());
    std::string session_request_response(&resp[0], &resp[session_request.response_len()]);
    std::cout << session_request_response << std::endl;
    // ---------------------- Get Session and Key ---------------------------

    std::cout << "'" << session << "' '" << key << "'" << std::endl;

    // ---------------------- Authenticate ---------------------------
    http_request auth_request;
    form_data auth_data;
    std::vector<uint8_t> cipher( (pass.size()/16 + 1)*16 );
    AES128_ECB_Enc( reinterpret_cast<const uint8_t*>(key.c_str()), 32,
                    reinterpret_cast<const uint8_t*>(pass.c_str()), pass.size(),
                    &cipher[0], cipher.size());
    char *base64str = base64(&cipher[0], cipher.size());
    std::string enc_pass = base64str;
    delete[] base64str;
    auth_data.add("user", user);
    auth_data.add("pass", enc_pass);
    auth_data.add("device_id", os_id);
    auth_data.add("device_name",os_id);
    auth_data.add("os_version",os_id);
    auth_data.add("os_type", os_id);
    auth_data.add("app_version", "0.01");
    std::string content_type = "multipart/form-data; boundary="+auth_data.boundrary().substr(2);
    auth_request.request_header("Content-Type", content_type);
    auth_request.request_header("SSBULSATAPI", session);
    auth_request.request_header("Origin","https://test.iptv.bulsat.com");
    auth_request.request_header("Referer","https://test.iptv.bulsat.com/iptv-login.php");
    auth_request.request_header("User-Agent", user_agent);
    std::string form_data_string = auth_data.string();
    res = auth_request.post(auth_url, reinterpret_cast<const uint8_t*>(form_data_string.c_str()), form_data_string.length());
    if(res != 200) {
        return 1;
    }
    resp = reinterpret_cast<const char*>(auth_request.response());
    std::string auth_request_response(&resp[0], &resp[auth_request.response_len()]);
//    std::cout << auth_request_response << std::endl;
    // ---------------------- Authenticate ---------------------------

    // ---------------------- Request Channels ---------------------------
    http_request channel_request;
    channel_request.request_header("SSBULSATAPI", session);
    channel_request.request_header("Origin","https://test.iptv.bulsat.com");
    channel_request.request_header("Referer","https://test.iptv.bulsat.com/iptv-login.php");
    channel_request.request_header("User-Agent", user_agent);
    channel_request.post(channel_url, reinterpret_cast<const uint8_t*>(""), 0);
    resp = reinterpret_cast<const char*>(channel_request.response());
    std::string channel_request_response(&resp[0], &resp[channel_request.response_len()]);
//    std::cout << channel_request_response << std::endl;
    // ---------------------- Request Channels ---------------------------

    // ---------------------- Logout ---------------------------
    http_request logout_request;
    form_data logout_data;
    logout_data.add("logout", "1");
    content_type = "multipart/form-data; boundary="+logout_data.boundrary().substr(2);
    logout_request.request_header("Content-Type", content_type);
    logout_request.request_header("SSBULSATAPI", session);
    logout_request.request_header("User-Agent", user_agent);
    form_data_string = logout_data.string();
    res = logout_request.post(auth_url, reinterpret_cast<const uint8_t*>(form_data_string.c_str()), form_data_string.length());
    if(res != 200) {
        return 1;
    }
    resp = reinterpret_cast<const char*>(logout_request.response());
    std::string logout_request_response(&resp[0], &resp[logout_request.response_len()]);
//    std::cout << logout_request_response << std::endl;
    // ---------------------- Logout ---------------------------

    // ---------------------- Generate Playlist ---------------------------
    Json::Reader reader;
    Json::Value channels;
    std::ofstream playlist_file(playlist_filepath_tmp.c_str());
    if(playlist_file.is_open()) {
        std::ostream &out = playlist_file;
        out << "#EXTM3U" << std::endl;
        if(reader.parse(channel_request_response, channels)) {
            if(channels.type() == Json::arrayValue) {
                for(int i = 0 ; i < channels.size() ; i++) {
                    Json::Value &channel = channels[i];
                    if(channel.isObject()) {
                        out << "#EXTINF:-1 group-title=\"" << channel["genre"].asString() << "\" tvg-id=\"" << channel["epg_name"].asString() << "\","<< channel["title"].asString() << std::endl;
                        out << channel["sources"].asString() << std::endl;
                    }
                }
            } else {
                return 1;
            }
        }
        playlist_file.close();
        rename(playlist_filepath_tmp.c_str(), playlist_filepath.c_str());
    } else {
        return 1;
    }
    // ---------------------- Generate Playlist ---------------------------
    return 0;
}
