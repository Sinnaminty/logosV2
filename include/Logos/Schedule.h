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
  void sort();
};

struct Schedule {
  std::vector<UserSchedule> m_schedules;
  void sort();
};

// front end functions
void scheduleAdd(const dpp::snowflake& snowflake,
                 const std::string& name,
                 const std::string& date,
                 const std::string& time);

void scheduleEdit(const dpp::snowflake& snowflake,
                  const std::string& name,
                  const std::string& date,
                  const std::string& time);

void scheduleRemove(const dpp::snowflake& snowflake,
                    const std::string& name,
                    const std::string& date,
                    const std::string& time);

// back end functions
Schedule initGlobalSchedule();
Schedule getGlobalSchedule();
void setGlobalSchedule(const Schedule& globalSchedule);
std::optional<std::pair<UserSchedule, ScheduleEntry>> checkGlobalSchedule();

UserSchedule initUserSchedule(const dpp::snowflake& userSnowflake);
UserSchedule getUserSchedule(const dpp::snowflake& userSnowflake);
void setUserSchedule(const UserSchedule& userSchedule);

time_t parseDateTime(const std::string& date, const std::string& time);
}  // namespace Schedule
