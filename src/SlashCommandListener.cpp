#include "SlashCommandListener.h"

#include <dpp/dpp.h>

#include "LogosSymphony.h"
#include "LogosUtility.h"

void SlashCommandListener::on_slashcommmand (
    const dpp::slashcommand_t &event ) {
    const std::string str = event.command.get_command_name ( );

    switch ( cmdHash ( str ) ) {
        case Command::WhoAmI: {
            LogosUtility::WhoAmI ( event );
            break;
        }

        case Command::Play: {
            LogosSymphony::Play ( event );
            break;
        }

        case Command::Pause: {
            LogosSymphony::Pause ( event );
            break;
        }

        case Command::Stop: {
            LogosSymphony::Stop ( event );
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

    else if ( str == "play" )
        return Command::Play;

    else if ( str == "pause" )
        return Command::Pause;

    else if ( str == "stop" )
        return Command::Stop;

    else
        return Command::NotValid;
}
