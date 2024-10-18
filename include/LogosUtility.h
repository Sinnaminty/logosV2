#pragma once
#include <dpp/dpp.h>

class LogosUtility {
   public:
    static void WhoAmI ( const dpp::slashcommand_t &event );

    static dpp::embed createEmbed ( const std::string &description );
};
