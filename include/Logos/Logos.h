#pragma once

#include <dpp/dpp.h>

using json = nlohmann::json;
namespace Logos {

enum class mType { GOOD, BAD, EVENT };

dpp::embed createEmbed(const mType& mType, const std::string& m);
std::vector<uint8_t> encodeAudio(const std::string& file);

}  // namespace Logos
