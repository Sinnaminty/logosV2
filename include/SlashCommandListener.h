#pragma once
#include <dpp/dispatcher.h>
class SlashCommandListener {
   public:
    static void on_slashcommmand ( const dpp::slashcommand_t &event );

   private:
    enum class Command { WhoAmI, Play, Pause, Stop, NotValid };
    static Command cmdHash ( const std::string &str );
};
