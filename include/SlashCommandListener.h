#pragma once
#include <dpp/dispatcher.h>
class SlashCommandListener {
   public:
    static void on_slashcommmand ( const dpp::slashcommand_t &event );

   private:
    enum cmd {
        cmdWhoAmI,
        cmdPing,
        cmdConnect,
        cmdDisconnect,
        cmdSchedule,
        cmdNotValid
    };
    static cmd cmdHash ( const std::string &str );
};
