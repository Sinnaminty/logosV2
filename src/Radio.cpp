#include <Logos/Radio.h>
#include <dpp/colors.h>
#include <mpg123.h>
#include <cstddef>

using json = nlohmann::json;

namespace Radio {
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

}  // namespace Radio
