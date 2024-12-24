#ifndef U_H
#define U_H

#include <dpp/dpp.h>
#include <string>
#include <vector>

namespace U {

enum class mType { GOOD, BAD };

struct Dice {
  int m_num;
  int m_side;

  std::string roll();
};

dpp::embed createEmbed(const U::mType& mType, const std::string& m);

std::string downloadSong(const std::string& link);
std::vector<uint8_t> encodeSong(const std::string& file);

void downloadSay(const std::string& text);
std::vector<uint8_t> encodeSay(const std::string& text);

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache);

std::vector<Dice> parseDiceString(const std::string& s);

}  // namespace U
#endif
