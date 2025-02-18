#include <Logos/Schedule.h>
#include <date/date.h>
#include <date/tz.h>
#include <dpp/cache.h>
#include <dpp/snowflake.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace Schedule {

/////////////////////////////////////////////////////
// ScheduleEntry
void from_json(const json& j, ScheduleEntry& entry) {
  j.at("eventName").get_to(entry.m_eventName);
  j.at("dateTime").get_to(entry.m_dateTime);
}

void to_json(json& j, const ScheduleEntry& entry) {
  j = json{{"eventName", entry.m_eventName}, {"dateTime", entry.m_dateTime}};
}

std::string ScheduleEntry::toString() const {
  std::ostringstream output;
  std::string eventName = this->m_eventName;
  long dateTime = this->m_dateTime;

  std::string formatedDateTime =
      dpp::utility::timestamp(dateTime, dpp::utility::tf_long_datetime);
  output << eventName << ": " << formatedDateTime << "\n";
  return output.str();
}

/////////////////////////////////////////////////////
// UserSchedule

void from_json(const json& j, UserSchedule& user) {
  j.at("events").get_to(user.m_events);
  j.at("snowflake").get_to(user.m_snowflake);
  j.at("timezone").get_to(user.m_timezone);
}

void to_json(json& j, const UserSchedule& user) {
  j = json{{"events", user.m_events},
           {"snowflake", user.m_snowflake},
           {"timezone", user.m_timezone}};
}

std::string UserSchedule::toString() const {
  std::ostringstream output;
  output << "Schedule for "
         << dpp::find_user(dpp::snowflake(this->m_snowflake))->username
         << ":\n";

  int i = 1;
  for (const auto& event : this->m_events) {
    output << i << ". " << event.toString();
    i++;
  }

  return output.str();
}

void UserSchedule::addEvent(const std::string& name,
                            const std::string& date,
                            const std::string& time) {
  m_events.push_back(
      ScheduleEntry{name, parseDateTime(date, time, m_timezone)});
  this->sort();
  setUserSchedule(*this);
}

void UserSchedule::removeEvent(const std::string& name) {
  auto eventIt = std::find_if(
      m_events.begin(), m_events.end(),
      [&](const ScheduleEntry& entry) { return entry.m_eventName == name; });
  if (eventIt == m_events.end()) {
    throw(std::runtime_error(
        "ERROR - UserSchedule::removeEvent: Event does not exist."));
  }

  // we good, ain't no pressure
  m_events.erase(eventIt);
  this->sort();
  setUserSchedule(*this);
}

void UserSchedule::removeEvent(const int& index) {
  if (index < 1 || index > m_events.size()) {
    throw(std::runtime_error("ERROR - UserSchedule::removeEvent: Index OOB"));
  }
  // assuming that 1 is the beginning
  m_events.erase(m_events.begin() + index - 1);
  this->sort();
  setUserSchedule(*this);
}

void UserSchedule::editEvent(const int& index,
                             const std::string& name,
                             const std::string& date,
                             const std::string& time) {
  if (index < 1 || index > m_events.size()) {
    throw(std::runtime_error("ERROR - UserSchedule::editEvent: Index OOB"));
  }
  auto& event = m_events[index - 1];
  if (name != "") {
    event.m_eventName = name;
  }
  if (date != "" && time != "") {
    event.m_dateTime = parseDateTime(date, time, m_timezone);
  }
  this->sort();
  setUserSchedule(*this);
}

void UserSchedule::setTimezone(const std::string& timezone) {
  this->m_timezone = timezone;
  setUserSchedule(*this);
}

void UserSchedule::sort() {
  std::sort(m_events.begin(), m_events.end(),
            [](const ScheduleEntry& a, const ScheduleEntry& b) {
              return a.m_dateTime < b.m_dateTime;
            });
  setUserSchedule(*this);
}

/////////////////////////////////////////////////////
// Schedule

void from_json(const json& j, Schedule& schedule) {
  j.at("schedules").get_to(schedule.m_schedules);
}

void to_json(json& j, const Schedule& schedule) {
  j = json{{"schedules", schedule.m_schedules}};
}

void Schedule::sort() {
  std::for_each(m_schedules.begin(), m_schedules.end(),
                [&](UserSchedule& userSchedule) { userSchedule.sort(); });
}

Schedule initGlobalSchedule() {
  std::ofstream outFile("json/schedule.json");

  if (outFile.is_open()) {
    json schj = Schedule();
    outFile << schj.dump(4) << std::endl;
    outFile.close();
    std::cout
        << "Debug - initGlobalSchedule: Successfully created schedule.json\n";

  } else {
    throw(
        std::runtime_error(" Error - initGlobalSchedule: Cannot create "
                           "schedule.json. you're fucked!"));
  }
  return Schedule();
}

// getters should always assume that the list is sorted
Schedule getGlobalSchedule() {
  if (!std::filesystem::exists("json/schedule.json")) {
    return initGlobalSchedule();
  }

  std::ifstream inFile("json/schedule.json");
  if (!inFile.is_open()) {
    throw(std::runtime_error(
        " Error - readSchedule: Cannot open schedule.json! You're fucked!"));
  }

  json schj;
  inFile >> schj;
  inFile.close();

  return schj.template get<Schedule>();
}

void setGlobalSchedule(const Schedule& globalSchedule) {
  if (!std::filesystem::exists("json/schedule.json")) {
    initGlobalSchedule();
  }

  std::ofstream outFile("json/schedule.json");
  if (!outFile.is_open()) {
    throw(std::runtime_error("Error writing to schedule file. You're fucked!"));
  }

  json j = globalSchedule;
  outFile << j.dump(4) << std::endl;
  outFile.close();
}

// this function handles the removal of the event if it exists.
std::optional<std::pair<UserSchedule, ScheduleEntry>> checkGlobalSchedule() {
  auto now = dpp::utility::time_f();
  Schedule globalSchedule = getGlobalSchedule();

  for (auto& u : globalSchedule.m_schedules) {
    for (auto& e : u.m_events) {
      if (e.m_dateTime < now) {
        auto retPair = std::pair<UserSchedule, ScheduleEntry>{u, e};
        u.removeEvent(e.m_eventName);
        return retPair;
      }
    }
  }

  // if we're here, no events were found!
  return std::nullopt;
}

UserSchedule initUserSchedule(const dpp::snowflake& userSnowflake,
                              const std::string& timezone) {
  Schedule globalSchedule = getGlobalSchedule();

  auto userScheduleIt = std::find_if(
      globalSchedule.m_schedules.begin(), globalSchedule.m_schedules.end(),
      [&](const UserSchedule& userSchedule) {
        return userSnowflake.str() == userSchedule.m_snowflake;
      });

  if (userScheduleIt != globalSchedule.m_schedules.end()) {
    throw(std::runtime_error(
        "ERROR - initUserSchedule: User is already in globalSchedule."));
  }
  // we know for sure that there is no UserSchedule.
  UserSchedule newUserSchedule{userSnowflake.str(),
                               std::vector<ScheduleEntry>(), timezone};
  globalSchedule.m_schedules.emplace_back(newUserSchedule);

  setGlobalSchedule(globalSchedule);
  return newUserSchedule;
}

UserSchedule getUserSchedule(const dpp::snowflake& userSnowflake) {
  Schedule globalSchedule = getGlobalSchedule();

  auto userScheduleIt = std::find_if(
      globalSchedule.m_schedules.begin(), globalSchedule.m_schedules.end(),
      [&](const UserSchedule& userSchedule) {
        return userSnowflake.str() == userSchedule.m_snowflake;
      });

  if (userScheduleIt == globalSchedule.m_schedules.end()) {
    return initUserSchedule(userSnowflake, "");
  }

  return *userScheduleIt;
}

void setUserSchedule(const UserSchedule& userSchedule) {
  Schedule globalSchedule = getGlobalSchedule();

  auto userScheduleIt = std::find_if(
      globalSchedule.m_schedules.begin(), globalSchedule.m_schedules.end(),
      [&](const UserSchedule& existingUserSchedule) {
        return existingUserSchedule.m_snowflake == userSchedule.m_snowflake;
      });

  if (userScheduleIt != globalSchedule.m_schedules.end()) {
    *userScheduleIt = userSchedule;

  } else {
    globalSchedule.m_schedules.emplace_back(userSchedule);
  }

  setGlobalSchedule(globalSchedule);
}

int64_t parseDateTime(const std::string& date,
                      const std::string& time,
                      const std::string& timezone) {
  // add some string santinization here, please.
  // this is like, really bad.
  std::string dateTime = date + " " + time;
  std::istringstream in{dateTime};
  date::local_time<std::chrono::minutes> localTimePoint;

  in >> date::parse("%m/%d/%y %H%M", localTimePoint);

  if (in.fail()) {
    throw std::runtime_error(
        "ERROR: parseDateTime: Failed to parse date time input.");
  }

  auto tz = date::locate_zone(timezone);

  date::zoned_time<std::chrono::minutes> userZonedTime{tz, localTimePoint};

  auto utcTime = userZonedTime.get_sys_time();

  return std::chrono::duration_cast<std::chrono::seconds>(
             utcTime.time_since_epoch())
      .count();
}

}  // namespace Schedule
