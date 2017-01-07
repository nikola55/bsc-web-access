#ifndef FORM_DATA_H
#define FORM_DATA_H

#include <string>
#include <vector>
#include <utility>

class form_data {
public:

    form_data();

    void add(const std::string &k, const std::string &v);

    const std::string& string() const;

    const std::string boundrary() const {
        return get_boundrary();
    }

protected:

    const char* get_boundrary() const;
    void generate_boundrary();

private:
    char form_boundrary[41];
    std::vector< std::pair<std::string, std::string> > fields;
    mutable std::string cache;
    mutable bool valid_cache;
};

#endif // FORM_DATA_H
