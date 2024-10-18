#include "LogosSymphony.h"

void LogosSymphony::Play ( const dpp::slashcommand_t &event ) {
    std::string url
        = std::get< std::string > ( event.get_parameter ( "link" ) );
    Connect ( event );
    // Some Other stuff
}

void LogosSymphony::Pause ( const dpp::slashcommand_t &event ) {}

void LogosSymphony::Stop ( const dpp::slashcommand_t &event ) {}

void LogosSymphony::Connect ( const dpp::slashcommand_t &event ) {
    dpp::guild *g = dpp::find_guild ( event.command.guild_id );
    // will return null if bot not in channel
    //
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
