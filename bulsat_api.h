#ifndef BULSAT_API_H
#define BULSAT_API_H

#include <string>

class BulsatAPI {
public:
    BulsatAPI(std::string cookiefile);
    void setUserCredentials(std::string user, std::string pass);
private:
    std::string m_cookiefile;
    std::string m_user;
    std::string m_pass;
};

#endif // BULSAT_API_H
