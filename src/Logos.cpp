#include <Logos/Logos.h>
#include <mpg123.h>
#include <cstddef>
#include <regex>

using json = nlohmann::json;

namespace Logos {

Carbon& Carbon::getInstance() {
  static Carbon instance;
  return instance;
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
