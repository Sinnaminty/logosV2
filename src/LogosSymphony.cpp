#include "LogosSymphony.h"

#include <dpp/dpp.h>

#include <cstdio>

void LogosSymphony::Play ( const dpp::slashcommand_t &event ) {
    std::string url
        = std::get< std::string > ( event.get_parameter ( "link" ) );
    std::string dlCommand = "yt-dlp -f bestaudio -o - " + url
                            + " | ffmpeg -i pipe:0 -f s16le -ar 48000 -ac 2 "
                              "-loglevel quiet pipe:1";
    FILE *stream = popen ( dlCommand.c_str ( ), "r" );

    if ( stream ) {
        dpp::voiceconn *vc = event.from->get_voice ( event.command.guild_id );
        if ( vc ) {
            event.reply ( "Started Streaming." );
            char buffer[ 4096 ];
            while ( fread ( buffer, sizeof ( char ), 4096, stream ) > 0 ) {
                vc->voiceclient->send_audio_raw ( ( uint16_t * ) buffer, 4096 );
            }
        }
        pclose ( stream );
    } else {
        event.reply ( "Could not stream this song." );
    }
}

void LogosSymphony::Pause ( const dpp::slashcommand_t &event ) {}

void LogosSymphony::Stop ( const dpp::slashcommand_t &event ) {}

void LogosSymphony::Connect ( const dpp::slashcommand_t &event ) {
    dpp::guild *guild = dpp::find_guild ( event.command.guild_id );
    dpp::voiceconn *currentVoiceChannel
        = event.from->get_voice ( event.command.guild_id );
    bool joinVoiceChannel = true;

    if ( currentVoiceChannel ) {
        // If we are in a voice channel at all...
        auto targetVoiceChannel = guild->voice_members.find (
            event.command.get_issuing_user ( ).id );

        if ( targetVoiceChannel != guild->voice_members.end ( )
             && currentVoiceChannel->channel_id
                    == targetVoiceChannel->second.channel_id ) {
            // If we are in the user's voice channel already...
            joinVoiceChannel = false;

        } else {
            event.from->disconnect_voice ( event.command.guild_id );
            joinVoiceChannel = true;
        }
    }

    if ( joinVoiceChannel ) {
        // If we need to join a voice channel..
        if ( ! guild->connect_member_voice (
                 event.command.get_issuing_user ( ).id ) ) {
            // If the user is not in any voice channel..
            event.reply ( "You're not in a VC, dummy!" );
            return;
        }
        event.reply ( "Joined your channel!" );

    } else {
        // We must be already in the channel!
        event.reply ( "I'm already here!" );
    }
}

void LogosSymphony::Disconnect ( const dpp::slashcommand_t &event ) {
    dpp::guild *guild = dpp::find_guild ( event.command.guild_id );
    dpp::voiceconn *currentVoiceChannel
        = event.from->get_voice ( event.command.guild_id );
    if ( currentVoiceChannel ) {
        // If we are in a voice channel at all...
        event.from->disconnect_voice ( event.command.guild_id );
    } else {
        event.reply ( "I'm not in a voice channel, dummy!" );
    }
}
