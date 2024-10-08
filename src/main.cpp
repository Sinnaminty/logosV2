#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include "MessageListener.h"
#include "SlashCommandListener.h"
using json = nlohmann::json;
int main ( int argc, const char *argv[] ) {
    json configDocument;
    std::ifstream configFile ( "../json/config.json" );
    configFile >> configDocument;
    dpp::snowflake ydsGuildId ( configDocument[ "yds-guild-id" ] );
    dpp::cluster bot ( configDocument[ "token" ],
                       dpp::i_default_intents | dpp::i_guild_members
                           | dpp::i_message_content );
    bot.on_log ( dpp::utility::cout_logger ( ) );
    bot.on_slashcommand ( &SlashCommandListener::on_slashcommmand );
    // bot.on_message_create ( &MessageListener::on_message_create );
    // bot.on_message_delete ( &MessageListener::on_message_delete );
    // bot.on_message_delete_bulk ( &MessageListener::on_message_delete_bulk );
    bot.on_ready ( [ &bot, &ydsGuildId ] ( const dpp::ready_t &event ) -> void {
        if ( dpp::run_once< struct clear_bot_commands > ( ) ) {
            // bot.global_bulk_command_delete();
            bot.guild_bulk_command_delete ( ydsGuildId );
        }
        if ( dpp::run_once< struct register_bot_commands > ( ) ) {
            dpp::slashcommand whoami ( "whoami",
                                       "Information about me.",
                                       bot.me.id );
            dpp::slashcommand ping ( "ping", "Wanna play?", bot.me.id );
            dpp::slashcommand connect ( "connect",
                                        "Make me join vc.",
                                        bot.me.id );
            dpp::slashcommand disconnect ( "disconnect",
                                           "Make me leave vc.",
                                           bot.me.id );
            dpp::slashcommand schedule ( "schedule",
                                         "Schedule an event.",
                                         bot.me.id );
            schedule.add_option ( dpp::command_option ( dpp::co_string,
                                                        "day",
                                                        "Day of the event.",
                                                        false ) );
            schedule.add_option ( dpp::command_option ( dpp::co_string,
                                                        "time",
                                                        "Time of the event.",
                                                        false ) );
            schedule.add_option ( dpp::command_option ( dpp::co_string,
                                                        "name",
                                                        "Name of the event.",
                                                        false ) );
            const std::vector< dpp::slashcommand > commands
                = { whoami, ping, connect, disconnect, schedule };
            bot.guild_bulk_command_create ( commands, ydsGuildId );
        }
    } );
    bot.start ( dpp::st_wait );
    return 0;
}
