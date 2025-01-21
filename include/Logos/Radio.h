#pragma once
#include <dpp/dpp.h>
using json = nlohmann::json;

namespace Radio {
std::string downloadSong(const std::string& link);
std::vector<uint8_t> encodeSong(const std::string& file);
}  // namespace Radio
