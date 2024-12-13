#include <dpp/cache.h>
#include <dpp/channel.h>
#include <dpp/dpp.h>
#include <dpp/message.h>

#include <algorithm>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

dpp::embed createEmbed ( const std::string &description ) {
    return dpp::embed ( )
        .set_color ( dpp::colors::pastel_light_blue )
        .set_title ( "Logos" )
        .set_url ( "https://fizzysylv.xyz/" )
        .set_author ( "Fizzy",
                      "https://fizzysylv.xyz/",
                      "https://genrandom.com/cats" )
        .set_description ( description )
        .set_thumbnail ( "https://genrandom.com/cats" )
        .add_field ( " field title", "Some value here" )
        .add_field ( "Inline field title", "Some value here", true )
        .add_field ( "Inline field title", "Some value here", true )
        .set_image ( "https://genrandom.com/cats" )
        .set_footer ( dpp::embed_footer ( ).set_text ( "meow~" ).set_icon (
            "https://genrandom.com/cats" ) )
        .set_timestamp ( time ( 0 ) );
}

void archiveChannel ( const std::string &channelName,
                      const dpp::cache< dpp::message > &cache ) {
    //                                        std::ofstream file ( channel.name
    //                                                             + ".txt" );
    //                                        if ( file.is_open ( ) ) {
    //                                            std::cout << "file get! \n";
    //                                            file << "[" <<
    //                                            msg.author.username
    //                                                 << "] " << ": " <<
    //                                                 msg.content
    //                                                 << "\n";
    //                                            file.close ( );
    //                                        } else {
    //                                            std::cerr << "Failed to create
    //                                            "
    //                                                         "file for
    //                                                         channel: "
    //                                                      << channel.name <<
    //                                                      "\n";
    //                                        }
}

int main ( int argc, const char *argv[] ) {
    json configDocument;
    std::ifstream configFile ( "../json/config.json" );
    configFile >> configDocument;

    dpp::snowflake ydsGuildId ( configDocument[ "yds-guild-id" ] );
    dpp::snowflake bosGuildId ( configDocument[ "bos-guild-id" ] );
    dpp::cluster bot ( configDocument[ "token" ],
                       dpp::i_default_intents | dpp::i_guild_members
                           | dpp::i_message_content );

    bot.on_log ( dpp::utility::cout_logger ( ) );
    bot.on_slashcommand ( [ & ] ( const dpp::slashcommand_t &event ) {
        if ( event.command.get_command_name ( ) == "whoami" ) {
            dpp::embed embed = createEmbed ( "Repetition Legitimizes." );
            dpp::message msg ( event.command.channel_id, embed );
            event.reply ( msg );
        } else if ( event.command.get_command_name ( ) == "archive" ) {
            dpp::snowflake guild_id = event.command.guild_id;

            // Fetch all channels in the guild
            bot.channels_get (
                guild_id,
                [ & ] ( const dpp::confirmation_callback_t &callback ) {
                    if ( callback.is_error ( ) ) {
                        std::cout << callback.get_error ( ).message << "\n";
                        return;
                    }

                    auto channels = callback.get< dpp::channel_map > ( );

                    std::cout << "got all channels!\n";

                    // Process all channels within the server.
                    for ( const auto &[ id, channel ] : channels ) {
                        if ( channel.is_text_channel ( ) ) {
                            std::cout << "is channel!: " << channel.name
                                      << "\n";
                            bot.messages_get (
                                id,
                                0,
                                0,
                                0,
                                100,
                                [ & ] ( const dpp::confirmation_callback_t
                                            &callback ) {
                                    if ( callback.is_error ( ) ) {
                                        std::cerr << "Failed to fetch messages "
                                                     "for channel: "
                                                  << channel.name << "\n";
                                        return;
                                    }

                                    auto messages
                                        = callback.get< dpp::message_map > ( );

                                    std::cout
                                        << "msgs get! channel:" << channel.name
                                        << "\n";

                                    dpp::cache< dpp::message > cache;
                                    for ( const auto &[ id, msg ] : messages ) {
                                        dpp::message *m = new dpp::message;
                                        *m = msg;
                                        cache.store ( m );
                                    }
                                    std::cout << "all messages saved for: "
                                              << channel.name
                                              << ". passing to archiveChannel.";

                                    archiveChannel ( channel.name, cache );
                                } );
                        }
                    }
                } );
        }
    } );

    bot.on_ready ( [ &bot, &ydsGuildId ] ( const dpp::ready_t &event ) -> void {
        if ( dpp::run_once< struct clear_bot_commands > ( ) ) {
            // bot.global_bulk_command_delete();
            bot.guild_bulk_command_delete ( ydsGuildId );
        }

        if ( dpp::run_once< struct register_bot_commands > ( ) ) {
            dpp::slashcommand whoami ( "whoami",
                                       "Information about Logos.",
                                       bot.me.id );

            dpp::slashcommand archive (
                "archive",
                "Saves every text channel in this server.",
                bot.me.id );

            const std::vector< dpp::slashcommand > commands
                = { whoami, archive };
            bot.guild_bulk_command_create ( commands, ydsGuildId );
            std::cout << "Bot Ready!\n";
        }
    } );

    bot.start ( dpp::st_wait );
    return 0;
}
