#ifndef _string_utils_hpp_
#define _string_utils_hpp_
#include <unordered_set>
#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>

namespace string_utils {
static inline bool split_string(const std::string& s,const std::string& delim_list,std::vector<std::string>& out,const bool skip_ws = false) {
    out.clear();
    std::string buf;

    std::unordered_set<char> set; //could use a  LUT/FLAG but w/e

    for (size_t i = 0;i < delim_list.size();++i)
        set.insert(delim_list[i]);

    size_t l = 0,m=s.length();

    //skip initial whitespace
    if (skip_ws) {
        while (l < m) {
            if (!isspace(s[l]))
                break;
            ++l;
        }
    }

    for (;l < m;++l) {
        const char ld = s[l];
        if (set.find(ld) != set.end()) {
            if (!buf.empty())
                out.push_back(buf);
            
            buf.clear();
            continue;
        }
        buf += ld;
    }

    if (!buf.empty())
        out.push_back(buf);

    return !out.empty();
}
 
static inline bool split_string(const std::string& s,const std::string& delim_list,std::vector<int>& out,const bool skip_ws = false){
    std::vector<std::string> tmp;
    split_string(s,delim_list,tmp,skip_ws);
    out.clear();
    if (tmp.empty())
        return false;

    try {
        for (size_t i = 0,j=tmp.size();i<j;++i)
            out.push_back( std::stoi(tmp[i]));
    } catch(...)  {
        return false;
    }
    return true;
}

static inline bool split_string(const std::string& s,const std::string& delim_list,std::vector<double>& out,const bool skip_ws = false){
    std::vector<std::string> tmp;
    split_string(s,delim_list,tmp,skip_ws);
    out.clear();
    if (tmp.empty())
        return false;

    try {
        for (size_t i = 0,j=tmp.size();i<j;++i)
            out.push_back( std::stod(tmp[i]));
    } catch(...)  {
        return false;
    }
    return true;
}
}
#endif