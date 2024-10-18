#include "SlashCommandListener.h"

#include <dpp/dpp.h>

#include "LogosSymphony.h"
#include "LogosUtility.h"

void SlashCommandListener::on_slashcommmand (
    const dpp::slashcommand_t &event ) {
    const std::string str = event.command.get_command_name ( );
    if ( str == "whoami" ) {
        LogosUtility::WhoAmI ( event );
    } else if ( str == "connect" ) {
        LogosSymphony::Connect ( event );
    } else if ( str == "disconnect" ) {
        LogosSymphony::Disconnect ( event );
    } else if ( str == "play" ) {
        LogosSymphony::Play ( event );
    } else if ( str == "pause" ) {
        LogosSymphony::Pause ( event );
    } else if ( str == "stop" ) {
        LogosSymphony::Stop ( event );
    } else {
        event.reply ( "Invalid command, dummy!" );
    }
}
