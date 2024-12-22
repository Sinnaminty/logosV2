#ifndef U_H
#define U_H

#include <dpp/dpp.h>

namespace U {

enum class mType { GOOD, BAD };

dpp::embed createEmbed(const U::mType& mType, const std::string& m);

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache);

bool match(const char* str, const char* mask);

/**
 *  trim from end of string (right)
 */
inline std::string rtrim(std::string s) {
  s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);

  return s;
}

/**
 * trim from beginning of string (left)
 */
inline std::string ltrim(std::string s) {
  s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
  return s;
}

/**
 * trim from both ends of string (right then left)
 */
inline std::string trim(std::string s) {
  return ltrim(rtrim(s));
}

}  // namespace U
#endif
