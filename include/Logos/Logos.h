#pragma once

#include <dpp/dpp.h>

using json = nlohmann::json;
namespace Logos {

enum class mType { GOOD, BAD, EVENT };

struct Dice {
  int m_num;
  int m_side;

  std::string roll();
};

class Carbon {
 public:
  static Carbon& getInstance();

  std::vector<std::vector<uint8_t>> s_songQueue;
  std::string s_currentSong;
  bool s_recording;

  std::map<dpp::snowflake, std::ofstream> s_userFileMap;

  Carbon(const Carbon&) = delete;

 protected:
  Carbon() = default;
};

dpp::embed createEmbed(const mType& mType, const std::string& m);

std::string downloadSong(const std::string& link);
std::vector<uint8_t> encodeSong(const std::string& file);

void downloadSay(const std::string& text);
std::vector<uint8_t> encodeSay(const std::string& text);

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache);

std::vector<Dice> parseDiceString(const std::string& s);

}  // namespace Logos
