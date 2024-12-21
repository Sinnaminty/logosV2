#ifndef U_H
#define U_H

#include <dpp/dpp.h>

namespace U {
    dpp::embed createEmbed ( const std::string &description );

    void archiveChannel ( const std::string &channelName,
                          const dpp::cache< dpp::message > &cache );
}  // namespace U
#endif
