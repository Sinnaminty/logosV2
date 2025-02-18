#pragma once

#include <dpp/dpp.h>
#include <dpp/snowflake.h>
#include <optional>
#include <string>

using json = nlohmann::json;
namespace Schedule {

struct ScheduleEntry {
  std::string m_eventName;
  long m_dateTime;

  std::string toString() const;
};

struct UserSchedule {
  std::string m_snowflake;
  std::vector<ScheduleEntry> m_events;
  std::string m_timezone;

  std::string toString() const;

  void addEvent(const std::string& name,
                const std::string& date,
                const std::string& time);

  void removeEvent(const std::string& name);
  void removeEvent(const int& index);

  void editEvent(const int& index,
                 const std::string& name,
                 const std::string& date,
                 const std::string& time);

  void setTimezone(const std::string& timezone);
  void sort();
};

struct Schedule {
  std::vector<UserSchedule> m_schedules;
  void sort();
};

Schedule initGlobalSchedule();
Schedule getGlobalSchedule();
void setGlobalSchedule(const Schedule& globalSchedule);
std::optional<std::pair<UserSchedule, ScheduleEntry>> checkGlobalSchedule();

UserSchedule initUserSchedule(const dpp::snowflake& userSnowflake,
                              const std::string& timezone);
UserSchedule getUserSchedule(const dpp::snowflake& userSnowflake);
void setUserSchedule(const UserSchedule& userSchedule);

int64_t parseDateTime(const std::string& date,
                      const std::string& time,
                      const std::string& timezone);
}  // namespace Schedule
