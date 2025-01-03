#pragma once

#include <dpp/dpp.h>

using json = nlohmann::json;
namespace Schedule {

struct ScheduleEntry {
  std::string m_eventName;
  long m_dateTime;
};

struct UserSchedule {
  std::string m_snowflake;
  std::vector<ScheduleEntry> m_events;
};

struct Schedule {
  std::vector<UserSchedule> m_schedules;
};

void initSchedule();
Schedule readSchedule();
void writeSchedule(const Schedule& schedule);
std::pair<dpp::snowflake, dpp::message> checkSchedule();

void scheduleAdd(const dpp::snowflake& snowflake,
                 const std::string& eventString);
void scheduleRemove(const ScheduleEntry& entryToRemove);
std::string scheduleShow(const dpp::snowflake& snowflake);

}  // namespace Schedule
