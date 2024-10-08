#pragma once
#include <dpp/dispatcher.h>
class SlashCommandListener {
   public:
    static void on_slashcommmand ( const dpp::slashcommand_t &event );

   private:
    enum class Command {
        WhoAmI,
        Ping,
        Connect,
        Disconnect,
        Schedule,
        NotValid
    };
    static Command cmdHash ( const std::string &str );
    static void cmdWhoAmI ( const dpp::slashcommand_t &event );
    static void cmdPing ( const dpp::slashcommand_t &event );
    static void cmdConnect ( const dpp::slashcommand_t &event );
    static void cmdDisconnect ( const dpp::slashcommand_t &event );
    static void cmdSchedule ( const dpp::slashcommand_t &event );
};
