#pragma once

#include <dpp/dpp.h>
#include <dpp/message.h>
#include <dpp/snowflake.h>
#include <dpp/user.h>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
namespace Logos {

enum class mType { GOOD, BAD };

struct Dice {
  int m_num;
  int m_side;

  std::string roll();
};

struct ScheduleEntry {
  std::string m_eventName;
  long m_dateTime;
};

struct ScheduleUser {
  std::vector<ScheduleEntry> m_events;
};

struct Schedule {
  std::map<dpp::snowflake, ScheduleUser> m_idUserMap;
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

std::pair<dpp::snowflake, dpp::message> checkSchedule();

void scheduleAdd(const dpp::snowflake& userId, const std::string& eventString);

std::string scheduleShow(const dpp::snowflake& userId);

dpp::embed createEmbed(const mType& mType, const std::string& m);

std::string downloadSong(const std::string& link);
std::vector<uint8_t> encodeSong(const std::string& file);

void downloadSay(const std::string& text);
std::vector<uint8_t> encodeSay(const std::string& text);

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache);

std::vector<Dice> parseDiceString(const std::string& s);

}  // namespace Logos
