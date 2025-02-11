#include <Logos/Logos.h>
#include <Logos/Vox.h>
namespace Vox {

void downloadSpeak(const std::string& text) {
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

std::vector<uint8_t> encodeSpeak(const std::string& text) {
  downloadSpeak(text);

  // output.mp3 now in project dir.
  auto ret = Logos::encodeAudio("say.mp3");

  return ret;
}

}  // namespace Vox
