#pragma once
#include <dpp/dispatcher.h>

class SlashCommandListener {
   public:
    static void on_slashcommmand ( const dpp::slashcommand_t &event );
};
