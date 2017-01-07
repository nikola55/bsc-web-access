#include "http_request.h"
#include <cstring>
#include <iostream>

http_request::http_request(const std::string &cf) : cookie_file(cf), curl(0), request_headers_buf(0) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    // check for 0
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &http_request::wirte_func<uint8_t>);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buf);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &http_request::wirte_func<char>);
    curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &headers_buf);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    if(cookie_file != "") {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_file.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_file.c_str());
    }
}


http_request::~http_request()  {
    if(request_headers_buf != 0) {
        curl_slist_free_all(request_headers_buf);
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

uint32_t http_request::post(const std::string &url, const uint8_t *payload, uint32_t sz_pay) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, sz_pay);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request_headers_buf);
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if(res != CURLE_OK) {
        return -1;
    }
    return http_code;
}

void http_request::request_header(const std::string & key, const std::string & value) {
    std::string header = key + ": " + value;
    request_headers_buf = curl_slist_append(request_headers_buf, header.c_str());
}

void http_request::clean_request_headers() {
    curl_slist_free_all(request_headers_buf);
    request_headers_buf = 0;
}

std::string http_request::response_header(const std::string & key) {
    if(headers_buf.size() > 0 && headers_buf[headers_buf.size()-1] != '\0') {
        headers_buf.push_back('\0'); // add trailing '\0' so
    }
    const char *match = strstr(&headers_buf[0], key.c_str());
    if(match != NULL) {
        match += key.size(); // advance with the size of the key
        // search for ':'
        while(*match != '\0' && *match != ':')
            match++;
        if(*match++ == ':') {
            // trim wihitespace
            while(*match != '\0' && isspace(*match))
                match++;

            if(*match == '\0')
                return "";

            const char * begin = match;
            while(*match != '\0' && *match != '\n')
                match++;

            if(*match == '\n') {
                return std::string(begin, match-1);
            }//
        }
    }
    return "";
}

const uint8_t * http_request::response() {
    return &response_buf[0];
}

uint32_t http_request::response_len() {
    return response_buf.size();
}

template <typename Byte>
size_t http_request::wirte_func(void *ptr, size_t size, size_t nmemb, void *userdata) {
    std::vector<Byte> *response_buf = static_cast<std::vector<Byte>*>(userdata);
    Byte *data = static_cast<Byte*>(ptr);
    response_buf->insert(response_buf->end(), &data[0], &data[nmemb*size]);
    return size*nmemb;
}
