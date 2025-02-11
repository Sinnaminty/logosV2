#pragma once
#include <dpp/dpp.h>

namespace Vox {

void downloadSpeak(const std::string& text);
std::vector<uint8_t> encodeSpeak(const std::string& text);

}  // namespace Vox
