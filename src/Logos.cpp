#include <Logos/Logos.h>
#include <dpp/cache.h>
#include <mpg123.h>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>
#include <string>
using json = nlohmann::json;

namespace Logos {

Carbon& Carbon::getInstance() {
  static Carbon instance;
  return instance;
}

// ScheduleEntry serialization and deserialization
void to_json(json& j, const ScheduleEntry& entry) {
  j = json{{"eventName", entry.m_eventName}, {"dateTime", entry.m_dateTime}};
}

void from_json(const json& j, ScheduleEntry& entry) {
  j.at("eventName").get_to(entry.m_eventName);
  j.at("dateTime").get_to(entry.m_dateTime);
}

// ScheduleUser serialization and deserialization
void to_json(json& j, const ScheduleUser& user) {
  j = json{{"events", user.m_events}};
}

void from_json(const json& j, ScheduleUser& user) {
  j.at("events").get_to(user.m_events);
}

// Schedule serialization and deserialization
void to_json(json& j, const Schedule& schedule) {
  j = json(schedule.m_idUserMap);
}

void from_json(const json& j, Schedule& schedule) {
  j.get_to(schedule.m_idUserMap);
}
dpp::embed createEmbed(const mType& mType, const std::string& m) {
  switch (mType) {
    case mType::GOOD: {
      return dpp::embed()
          .set_color(dpp::colors::iguana_green)
          .set_title("Success!")
          .set_description(m)
          .set_timestamp(time(0));
    }

    case mType::BAD: {
      return dpp::embed()
          .set_color(dpp::colors::cranberry)
          .set_title("Uh-Oh!")
          .set_description(m)
          .set_timestamp(time(0));
    }
    default: {
      return dpp::embed();
      break;
    }
  }
}

std::string toISO8601(const std::string& date, const std::string& time) {
  std::ostringstream isoFormat;
  std::istringstream dateStream(date);
  std::istringstream timeStream(time);

  int month, day, year, hour, minute, hourMinute;
  char slash;

  if (!(dateStream >> month >> slash >> day >> slash >> year) ||
      !(timeStream >> hourMinute)) {
    return "";  // Return empty string on failure
  }

  // Convert year to four digits
  year += (year < 50) ? 2000 : 1900;

  hour = hourMinute / 100;
  minute = hourMinute % 100;
  // Format as ISO 8601: YYYY-MM-DDTHH:MM
  isoFormat << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2)
            << month << "-" << std::setw(2) << day << "T" << std::setw(2)
            << hour << ":" << std::setw(2) << minute;

  return isoFormat.str();
}

void scheduleAdd(const dpp::snowflake& userId, const std::string& eventString) {
  // Parse input string
  std::istringstream iss(eventString);
  std::string eventName, date, time;

  if (!(iss >> eventName >> date >> time)) {
    throw(std::invalid_argument("Invalid input format."));
  }

  // Validate date format (mm/dd/yy)
  if (date.size() != 8 || date[2] != '/' || date[5] != '/') {
    throw(std::invalid_argument("Invalid date format."));
  }

  // Validate time format (24H time)
  if (time.size() != 4) {
    throw(std::invalid_argument("Invalid time format."));
  }

  // Convert date and time to ISO 8601 format
  std::string isoTimestamp = toISO8601(date, time);
  if (isoTimestamp.empty()) {
    throw(std::runtime_error(
        "Error converting date and time to ISO 8601 format."));
  }

  // Prepare JSON structure
  std::ifstream inFile("json/schedule.json");
  json schj;

  // Load existing data if file exists
  if (inFile.is_open()) {
    try {
      inFile >> schj;
    } catch (const json::exception& e) {
      std::cerr << e.what();
    }
    inFile.close();
  } else {
    // Create the file if it doesn't exist
    std::ofstream outFile("json/schedule.json");
    if (!outFile.is_open()) {
      throw(std::runtime_error("Error creating schedule file."));
    }
    outFile.close();
  }

  // Add new event
  ScheduleEntry newEntry{eventName, isoTimestamp};
  if (!schj.contains(std::to_string(userId))) {
    // If the user doesn't exist in the JSON, create their entry
    schj[std::to_string(userId)] = ScheduleUser{{}};
  }
  // Update the user's events
  auto& userJson = schj[std::to_string(userId)];
  userJson["events"].push_back(newEntry);

  // Write back to file
  std::ofstream outFile("json/schedule.json");
  if (!outFile.is_open()) {
    throw(std::runtime_error("Error writing to schedule file."));
  }

  outFile << std::setw(4) << schj << std::endl;
  std::cout << "schedule add complete!";
  outFile.close();
}

// Function to display the schedule for a given user
std::string scheduleShow(const dpp::snowflake& userId) {
  std::ifstream inFile("json/schedule.json");
  json schj;

  if (!inFile.is_open()) {
    throw(std::runtime_error("Schedule file not found."));
  }

  try {
    inFile >> schj;
  } catch (const std::exception& e) {
    std::cerr << e.what();
  }

  inFile.close();

  std::string username = dpp::find_user(userId)->username;
  if (schj.find(std::to_string(userId)) == schj.end()) {
    throw(std::runtime_error("No events found for user " + username + "."));
  }

  std::ostringstream output;
  output << "Schedule for " << username << ":\n";

  auto userSchedule = schj[std::to_string(userId)]["events"];
  for (const auto& event : userSchedule) {
    std::string eventName = event.value("eventName", "Unknown");
    std::string dateTime = event.value("dateTime", "Unknown");

    // Convert ISO 8601 to human-readable format
    std::istringstream tsStream(dateTime);
    std::tm tm = {};
    tsStream >> std::get_time(&tm, "%Y-%m-%dT%H:%M");

    if (!tsStream.fail()) {
      char buffer[100];
      std::strftime(buffer, sizeof(buffer), "%B %d, %Y at %H:%M", &tm);
      output << "- " << eventName << " on " << buffer << "\n";
    } else {
      output << "- " << eventName << " at " << dateTime << "\n";
    }
  }

  return output.str();
}

std::vector<uint8_t> encodeSong(const std::string& file) {
  std::vector<uint8_t> pcmdata;

  mpg123_init();

  int err;
  mpg123_handle* mh = mpg123_new(NULL, &err);
  unsigned char* buffer;
  size_t buffer_size;
  size_t done;
  int channels, encoding;
  long rate;

  mpg123_param(mh, MPG123_FORCE_RATE, 48000, 48000.0);

  buffer_size = mpg123_outblock(mh);
  buffer = new unsigned char[buffer_size];

  mpg123_open(mh, file.c_str());
  mpg123_getformat(mh, &rate, &channels, &encoding);

  unsigned int counter = 0;
  for (int totalBtyes = 0;
       mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK;) {
    for (auto i = 0; i < buffer_size; i++) {
      pcmdata.push_back(buffer[i]);
    }
    counter += buffer_size;
    totalBtyes += done;
  }
  delete buffer;
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
  return pcmdata;
}

void downloadSay(const std::string& text) {
  if (std::filesystem::exists("say.wav")) {
    std::filesystem::remove("say.wav");
  }
  if (std::filesystem::exists("say.mp3")) {
    std::filesystem::remove("say.mp3");
  }

  std::string command = "say -pre \"[:phoneme on] \" -e 1 -fo say.wav -a \"";
  command += text + "\"";

  int retCode = std::system(command.c_str());
  if (retCode != 0) {
    throw std::runtime_error("Error: Failed to generate text.");
  }

  // say.wav now in project dir.

  command.clear();
  command =
      "ffmpeg -f wav -i say.wav -ar 48000 -ac 2 "
      "say.mp3";
  retCode = std::system(command.c_str());
  if (retCode != 0) {
    throw std::runtime_error("Error: ffmpeg error.");
  }
}

std::vector<uint8_t> encodeSay(const std::string& text) {
  downloadSay(text);

  // output.mp3 now in project dir.
  auto ret = encodeSong("say.mp3");

  return ret;
}

std::string downloadSong(const std::string& link) {
  const std::string outputDir = "music";

  if (std::filesystem::exists(outputDir)) {
    std::filesystem::remove_all(outputDir);
  }

  std::filesystem::create_directory(outputDir);
  // Command to download the MP3 using yt-dlp
  std::string command = "yt-dlp --extract-audio --audio-format mp3 ";
  command += "--output \"" + outputDir + "/%(title)s.%(ext)s\" ";
  command += "\"" + link + "\"";

  // Execute the command
  int retCode = std::system(command.c_str());
  if (retCode != 0) {
    throw std::runtime_error("Error: Failed to download the MP3 using yt-dlp.");
  }

  // Find the latest file in the "music" directory
  std::string latestFile;
  auto latestTime = std::filesystem::file_time_type::min();

  for (const auto& entry : std::filesystem::directory_iterator(outputDir)) {
    if (entry.is_regular_file()) {
      auto lastWriteTime = entry.last_write_time();
      if (lastWriteTime > latestTime) {
        latestTime = lastWriteTime;
        latestFile = entry.path().filename().string();
      }
    }
  }

  if (latestFile.empty()) {
    throw std::runtime_error("Error: Could not determine the downloaded file.");
  }

  return latestFile;
}

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache) {}

std::string Dice::roll() {
  srand((unsigned)time(NULL));
  std::string retString;

  for (int i = 0; i < this->m_num; i++) {
    retString += std::to_string(1 + (rand() % this->m_side)) + " ";
  }
  return retString;
}

std::vector<Dice> parseDiceString(const std::string& s) {
  std::vector<Dice> diceList;
  std::regex diceRegex(R"((\d+)d(\d+))");
  std::sregex_iterator begin(s.begin(), s.end(), diceRegex);
  std::sregex_iterator end;

  for (auto it = begin; it != end; ++it) {
    int num = std::stoi((*it)[1].str());
    int sides = std::stoi((*it)[2].str());

    if (num <= 0 || sides <= 0) {
      throw std::invalid_argument(
          "Dice must have positive numbers for quantity and sides.");
    }

    diceList.push_back({num, sides});
  }

  return diceList;
}

}  // namespace Logos
