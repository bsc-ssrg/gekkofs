#ifndef IFS_PATH_UTIL_HPP
#define IFS_PATH_UTIL_HPP

#include <string>
#include <vector>

#define PATH_MAX_LEN 4096 // 4k chars

const constexpr char PSP('/'); // PATH SEPARATOR


bool is_relative_path(const std::string& path);
bool is_absolute_path(const std::string& path);
bool has_trailing_slash(const std::string& path);

std::string prepend_path(const std::string& path, const char * raw_path);
std::string dirname(const std::string& path);
std::vector<std::string> split_path(const std::string& path);

#endif //IFS_PATH_UTIL_HPP
