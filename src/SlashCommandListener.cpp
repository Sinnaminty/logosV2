#include "SlashCommandListener.h"
#include <dpp/dpp.h>
void SlashCommandListener::on_slashcommmand (
    const dpp::slashcommand_t &event ) {
    const std::string str = event.command.get_command_name ( );
    switch ( cmdHash ( str ) ) {
        case Command::WhoAmI: {
            cmdWhoAmI ( event );
            break;
        }
        case Command::Ping: {
            cmdPing ( event );
            break;
        }
        case Command::Connect: {
            cmdConnect ( event );
            break;
        }
        case Command::Disconnect: {
            cmdDisconnect ( event );
            break;
        }
        case Command::Schedule: {
            cmdSchedule ( event );
            break;
        }
        case Command::NotValid: {
            event.reply ( "Invalid Command, dummy :p" );
            break;
        }
    }
}
SlashCommandListener::Command SlashCommandListener::cmdHash (
    const std::string &str ) {
    if ( str == "whoami" ) return Command::WhoAmI;
    else if ( str == "ping" )
        return Command::Ping;
    else if ( str == "connect" )
        return Command::Connect;
    else if ( str == "disconnect" )
        return Command::Disconnect;
    else if ( str == "schedule" )
        return Command::Schedule;
    else
        return Command::NotValid;
}
void SlashCommandListener::cmdWhoAmI ( const dpp::slashcommand_t &event ) {
    dpp::embed embed
        = dpp::embed ( )
              .set_color ( dpp::colors::pastel_light_blue )
              .set_title ( "Logos" )
              .set_url ( "https://fizzysylv.xyz/" )
              .set_author ( "Fizzy",
                            "https://fizzysylv.xyz/",
                            "https://genrandom.com/cats" )
              .set_description ( "Repitition Legitimizes." )
              .set_thumbnail ( "https://genrandom.com/cats" )
              .add_field ( " field title", "Some value here" )
              .add_field ( "Inline field title", "Some value here", true )
              .add_field ( "Inline field title", "Some value here", true )
              .set_image ( "https://genrandom.com/cats" )
              .set_footer (
                  dpp::embed_footer ( ).set_text ( "meow~" ).set_icon (
                      "https://genrandom.com/cats" ) )
              .set_timestamp ( time ( 0 ) );
    dpp::message msg ( event.command.channel_id, embed );
    event.reply ( msg );
}
void SlashCommandListener::cmdPing ( const dpp::slashcommand_t &event ) {
    event.reply ( "Pong!" );
}
void SlashCommandListener::cmdConnect ( const dpp::slashcommand_t &event ) {
    dpp::guild *g = dpp::find_guild ( event.command.guild_id );
    // will return null if bot not in channel
    auto current_vc = event.from->get_voice ( event.command.guild_id );
    bool join_vc = true;
    if ( current_vc ) {
        // Find the channel id that the issuing user is currently in
        auto users_vc
            = g->voice_members.find ( event.command.get_issuing_user ( ).id );
        if ( users_vc != g->voice_members.end ( )
             && current_vc->channel_id == users_vc->second.channel_id ) {
            join_vc = false;
            /* We are on this voice channel, at this point we can
             send any audio instantly to vc:

             * current_vc->send_audio_raw(...)
             */
        } else {
            /* We are on a different voice channel. We should leave
             * it, then join the new one by falling through to the
             * join_vc branch below.
             */
            event.from->disconnect_voice ( event.command.guild_id );
            join_vc = true;
        }
    }
    /* If we need to join a vc at all, join it here if join_vc ==
     * true
     */
    if ( join_vc ) {
        /* Attempt to connect to a voice channel, returns false if
         * we fail to connect. */
        /* The user issuing the command is not on any voice channel,
         * we can't do anything */
        if ( ! g->connect_member_voice (
                 event.command.get_issuing_user ( ).id ) ) {
            event.reply ( "You don't seem to be in a voice channel!" );
            return;
        }
        /* We are now connecting to a vc. Wait for on_voice_ready
         * event, and then send the audio within that event:
         *
         * event.voice_client->send_audio_raw(...);
         *
         * NOTE: We can't instantly send audio, as we have to wait
         * for the connection to the voice server to be established!
         */
        /* Tell the user we joined their channel. */
        event.reply ( "Joined your channel!" );
    } else {
        event.reply ( "I'm already here!" );
    }
}
void SlashCommandListener::cmdDisconnect ( const dpp::slashcommand_t &event ) {
    event.reply ( "Not yet implemented! Working on it.." );
}
void SlashCommandListener::cmdSchedule ( const dpp::slashcommand_t &event ) {
    dpp::snowflake user_id = event.command.usr.id;
}
