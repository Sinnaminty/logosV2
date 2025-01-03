#include <Logos/Schedule.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace Schedule {

// ScheduleEntry
void from_json(const json& j, ScheduleEntry& entry) {
  j.at("eventName").get_to(entry.m_eventName);
  j.at("dateTime").get_to(entry.m_dateTime);
}

void to_json(json& j, const ScheduleEntry& entry) {
  j = json{{"eventName", entry.m_eventName}, {"dateTime", entry.m_dateTime}};
}

/////////////////////////////////////////////////////

// UserSchedule

void from_json(const json& j, UserSchedule& user) {
  j.at("events").get_to(user.m_events);
  j.at("snowflake").get_to(user.m_snowflake);
}

void to_json(json& j, const UserSchedule& user) {
  j = json{{"events", user.m_events}, {"snowflake", user.m_snowflake}};
}

/////////////////////////////////////////////////////

// Schedule

void from_json(const json& j, Schedule& schedule) {
  j.at("schedules").get_to(schedule.m_schedules);
}

void to_json(json& j, const Schedule& schedule) {
  j = json{{"schedules", schedule.m_schedules}};
}

/////////////////////////////////////////////////////

void initSchedule() {
  std::ofstream outFile("json/schedule.json");

  if (outFile.is_open()) {
    json schj = Schedule();
    outFile << schj.dump(4) << std::endl;
    outFile.close();
    std::cout << "Debug - initSchedule: Successfully created schedule.json\n";

  } else {
    throw(std::runtime_error(
        " Error - initSchedule : Cannot create schedule.json. you're fucked!"));
  }
}

Schedule readSchedule() {
  if (!std::filesystem::exists("json/schedule.json")) {
    initSchedule();
  }

  std::ifstream inFile("json/schedule.json");
  if (!inFile.is_open()) {
    throw(std::runtime_error(
        " Error - readSchedule: Cannot open schedule.json!"));
  }

  json schj;
  inFile >> schj;
  inFile.close();

  return schj.template get<Schedule>();
}

void writeSchedule(const Schedule& schedule) {
  if (!std::filesystem::exists("json/schedule.json")) {
    initSchedule();
  }
  // Write back to file
  std::ofstream outFile("json/schedule.json");
  if (!outFile.is_open()) {
    throw(std::runtime_error("Error writing to schedule file."));
  }

  json j = schedule;

  outFile << j.dump(4) << std::endl;
  std::cout << "schedule add complete!";
  outFile.close();
}

std::pair<dpp::snowflake, dpp::message> checkSchedule() {
  std::pair<dpp::snowflake, dpp::message> ret;
  auto now = dpp::utility::time_f();

  Schedule sch = readSchedule();
  for (auto u : sch.m_schedules) {
    for (auto e : u.m_events) {
      if (e.m_dateTime < now) {
        ret.first = dpp::snowflake(u.m_snowflake);
        ret.second = dpp::message(e.m_eventName);
      }
    }
  }

  return ret;
}

void scheduleAdd(const dpp::snowflake& userId, const std::string& eventString) {
  // Parse input string
  std::istringstream iss(eventString);
  std::string eventName, date, time;

  if (!(iss >> eventName >> date >> time)) {
    throw(std::invalid_argument("Invalid input format."));
  }

  auto dateVec = dpp::utility::tokenize(date, "/");

  int year = std::stoi(dateVec.at(2));

  year += 100;

  int month = std::stoi(dateVec.at(0));

  int day = std::stoi(dateVec.at(1));
  day--;

  int realTime = std::stoi(time);
  int hours = realTime / 100;
  int minutes = realTime % 100;

  // Convert date and time to unix timestamp
  struct tm datetime;

  datetime.tm_year = year;
  datetime.tm_mon = month;
  datetime.tm_mday = day;
  datetime.tm_hour = hours;
  datetime.tm_min = minutes;
  datetime.tm_sec = 0;
  datetime.tm_isdst = -1;

  time_t timestamp = mktime(&datetime);

  Schedule sch = readSchedule();
  ScheduleEntry newEntry{eventName, timestamp};

  auto it = std::find_if(
      sch.m_schedules.begin(), sch.m_schedules.end(),
      [&](const UserSchedule& u) { return (u.m_snowflake == userId.str()); });

  if (it != sch.m_schedules.end()) {
    it->m_events.push_back(newEntry);

  } else {
    UserSchedule uSch;
    uSch.m_snowflake = userId.str();
    uSch.m_events.push_back(newEntry);
    sch.m_schedules.push_back(uSch);
  }
  writeSchedule(sch);
}

void scheduleRemove(const UserSchedule& userSchedule,
                    const ScheduleEntry& eventToRemove) {
  Schedule sch = readSchedule();
  auto it = std::find_if(sch.m_schedules.begin(), sch.m_schedules.end(),
                         [&](const UserSchedule& u) {
                           return (u.m_snowflake == userSchedule.m_snowflake);
                         });
}

std::string scheduleShow(const dpp::snowflake& userId) {
  Schedule sch = readSchedule();

  auto it = std::find_if(
      sch.m_schedules.begin(), sch.m_schedules.end(),
      [&](const UserSchedule& u) { return (u.m_snowflake == userId.str()); });

  std::string username = dpp::find_user(userId)->username;

  if (it == sch.m_schedules.end()) {
    throw(std::runtime_error("No events found for user " + username + "."));
  }

  std::ostringstream output;
  output << "Schedule for " << username << ":\n";

  for (const auto& event : it->m_events) {
    std::string eventName = event.m_eventName;
    long dateTime = event.m_dateTime;

    std::string formatedDateTime =
        dpp::utility::timestamp(dateTime, dpp::utility::tf_long_datetime);
    output << eventName << ": " << formatedDateTime << "\n";
  }

  return output.str();
}

}  // namespace Schedule
