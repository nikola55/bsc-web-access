#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <stdint.h>
#include <curl/curl.h>
#include <string>
#include <vector>

class http_request {

    http_request(const http_request &) { }

    http_request& operator=(const http_request &) { return *this; }

public:

    http_request(const std::string &cf = "");

    ~http_request();

    void get(const std::string & url);

    uint32_t post(const std::string & url, const uint8_t *payload, uint32_t sz_pay);

    void request_header(const std::string & key, const std::string & value);

    void clean_request_headers();

    std::string response_header(const std::string & key);

    const uint8_t * response();

    uint32_t response_len();

private:
    std::vector<uint8_t> response_buf;
    std::vector<char> headers_buf;
    std::string cookie_file;
    CURL *curl;
    curl_slist *request_headers_buf;
private:
    template <typename Byte>
    static size_t wirte_func(void *ptr, size_t size, size_t nmemb, void *userdata);
};

#endif // HTTP_REQUEST_H
