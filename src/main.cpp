#include <dpp/dpp.h>

#include <dpp/nlohmann/json.hpp>

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
    //
    bot.on_ready ( [ &bot, &ydsGuildId ] ( const dpp::ready_t &event ) -> void {
        if ( dpp::run_once< struct clear_bot_commands > ( ) ) {
            // bot.global_bulk_command_delete();
            bot.guild_bulk_command_delete ( ydsGuildId );
        }

        if ( dpp::run_once< struct register_bot_commands > ( ) ) {
            dpp::slashcommand whoami ( "whoami",
                                       "Information about Logos.",
                                       bot.me.id );

            dpp::slashcommand connect ( "connect",
                                        "Connect me to a VC.",
                                        bot.me.id );

            dpp::slashcommand disconnect ( "disconnect",
                                           "Disconnect me from a VC.",
                                           bot.me.id );

            dpp::slashcommand play (
                "play",
                "Play audio from a Youtube link, connects bot to the voice "
                "channel that the user is currently in.",
                bot.me.id );
            play.add_option ( dpp::command_option (
                dpp::co_string,
                "link",
                "Link of the Youtube video to play in a voice channel.",
                true ) );

            dpp::slashcommand pause (
                "pause",
                "Pause the music playing in place if there's any.",
                bot.me.id );

            dpp::slashcommand stop (
                "stop",
                "Stop the music playing if there's any, disconnects bot.",
                bot.me.id );

            const std::vector< dpp::slashcommand > commands
                = { whoami, connect, disconnect, play, pause, stop };
            bot.guild_bulk_command_create ( commands, ydsGuildId );
        }
    } );

    bot.start ( dpp::st_wait );
    return 0;
}
