#ifndef U_H
#define U_H

#include <dpp/dpp.h>

namespace U {

enum class mType { GOOD, BAD };

dpp::embed createEmbed(const U::mType& mType, const std::string& m);

std::vector<uint8_t> encodeSong(std::string file);

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache);

}  // namespace U
#endif
