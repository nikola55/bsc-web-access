#include "form_data.h"
#include <cstdio>
#include <string>
#include <iostream>
#include <time.h>
#include <stdlib.h>

form_data::form_data() : valid_cache(true) {
    generate_boundrary();
}

void form_data::add(const std::string &k, const std::string &v) {
    fields.push_back(std::make_pair(k,v));
    valid_cache = false;
}

const char * form_data::get_boundrary() const {
    return form_boundrary;
}

const std::string& form_data::string() const {
    if(valid_cache)
        return cache;
    cache.clear();
    cache.reserve(2*41*(fields.size()+1) + 3);
    for(unsigned i = 0 ; i < fields.size() ; i++) {
        cache += form_boundrary;
        cache += '\n';
        cache += "Content-Disposition: form-data; name=\"";
        cache += fields[i].first + "\"\n\n";
        cache += fields[i].second;
        cache += "\n";
    }
    cache += form_boundrary;
    cache += "--";
    valid_cache = true;
    return cache;
}

void form_data::generate_boundrary() {
    static const char * alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    srand(time(NULL));
    char form_data_id[17];
    for(unsigned i = 0 ;  i < 16 ; i++) {
        unsigned r = rand();
        r %= 52;
        form_data_id[i]=alphabet[r];
    }
    form_data_id[16] = '\0';

    std::sprintf(form_boundrary, "------WebKitFormBoundary%s", form_data_id);

}


