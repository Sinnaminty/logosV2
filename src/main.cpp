#include "Listeners.h"
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

static const long YDS_GUILD_ID = 920188312591933460;

using json = nlohmann::json;

int main(int argc, char const *argv[]) {
  json configDocument;
  std::ifstream configFile("../config.json");
  configFile >> configDocument;
  dpp::cluster bot(configDocument["token"], dpp::i_default_intents |
                                                dpp::i_guild_members |
                                                dpp::i_message_content);

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand(&SlashCommandListener::on_slashcommmand);

  bot.on_ready([&bot](const dpp::ready_t &event) -> void {
    if (dpp::run_once<struct clear_bot_commands>()) {
      // bot.global_bulk_command_delete();
      bot.guild_bulk_command_delete(YDS_GUILD_ID);
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand whoami("whoami", "Information about me.", bot.me.id);
      dpp::slashcommand ping("ping", "Wanna play?", bot.me.id);
      dpp::slashcommand connect("connect", "Make me join vc.", bot.me.id);
      dpp::slashcommand disconnect("disconnect", "Make me leave vc.",
                                   bot.me.id);

      const std::vector<dpp::slashcommand> commands = {whoami, ping, connect,
                                                       disconnect};
      bot.guild_bulk_command_create(commands, YDS_GUILD_ID);
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
