#pragma once
#include <dpp/dpp.h>

class LogosSymphony {
   public:
    static void Play ( const dpp::slashcommand_t &event );

    static void Pause ( const dpp::slashcommand_t &event );

    static void Stop ( const dpp::slashcommand_t &event );

   private:
    static void Connect ( const dpp::slashcommand_t &event );
};
