#include "Listeners.h"
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

static const long YDS_GUILD_ID = 920188312591933460;

using json = nlohmann::json;

int main(int argc, char const *argv[]) {
  json configDocument;
  std::ifstream configFile("../config.json");
  configFile >> configDocument;
<<<<<<< HEAD
  dpp::cluster bot(configDocument["token"],
                   dpp::i_default_intents | dpp::i_guild_members);
=======
  dpp::cluster bot(configDocument["token"], dpp::i_default_intents |
                                                dpp::i_guild_members |
                                                dpp::i_message_content);
  bot.on_log(dpp::utility::cout_logger());

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> rand(1, 1000);

  /* The event is fired when the bot detects a message in any server and any
   * channel it has access to. */
  bot.on_message_create(
      [&bot, &rng, &rand](const dpp::message_create_t &event) {
        /* 1/500 chance. */
        if (event.msg.content.find("hoe") != std::string::npos) {
          if (rand(rng) % 500 == 0) {
            event.reply("hoes mad?", true);
          }
        }
      });

  bot.on_message_reaction_remove(
      [&bot](const dpp::message_reaction_remove_t &event) {
        // Find the user in the cache using his discord id
        dpp::user *reacting_user = dpp::find_user(event.reacting_user_id);

        // If user not found in cache, log and return
        if (!reacting_user) {
          bot.log(dpp::ll_info, "User with the id " +
                                    std::to_string(event.reacting_user_id) +
                                    " was not found.");
          return;
        }

        bot.log(dpp::ll_info,
                reacting_user->format_username() + " removed his reaction.");
      });

  bot.on_slashcommand([&bot](const dpp::slashcommand_t &event) {
    if (event.command.get_command_name() == "msgs-get") {
      int64_t limit = std::get<int64_t>(event.get_parameter("quantity"));

      // get messages using ID of the channel the command was issued in
      bot.messages_get(event.command.channel_id, 0, 0, 0, limit,
                       [event](const dpp::confirmation_callback_t &callback) {
                         if (callback.is_error()) {
                           std::cout << callback.get_error().message
                                     << std::endl;
                           return;
                         }
                         // std::get<dpp::message_map>(callback.value) would
                         // give the same result
                         auto messages = callback.get<dpp::message_map>();
                         std::string contents;

                         // where x.first is ID of the current message and
                         // x.second is the message itself.
                         for (const auto &x : messages) {
                           contents += x.second.content + '\n';
                         }
                         event.reply(contents);
                       });

    } else if (event.command.get_command_name() == "pfp") {
      dpp::snowflake user;

      // If there was no specified user, we set the "user" variable to the
      // command author.
      if (event.get_parameter("user").index() == 0) {
        user = event.command.get_issuing_user().id;
      } else {
        user = std::get<dpp::snowflake>(event.get_parameter("user"));
      }
      // attach the user's name and profile picture to the embed
      dpp::embed embed;
      dpp::user u = event.command.bot->user_get_sync(user);

      // Attach the user's name and profile picture to the embed
      embed.set_title(u.username + "'s Profile Picture")
          .set_image(u.get_avatar_url())
          .set_color(dpp::colors::blue);
      dpp::message msg(event.command.channel_id, embed);
      event.reply(msg);
>>>>>>> a4274b0421ce1a8fafc12dfeb4f49a5f2f7fe886

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
<<<<<<< HEAD
      dpp::slashcommand connect("connect", "Make me join vc.", bot.me.id);
      dpp::slashcommand disconnect("disconnect", "Make me leave vc.",
                                   bot.me.id);

      const std::vector<dpp::slashcommand> commands = {whoami, ping, connect,
                                                       disconnect};
      bot.guild_bulk_command_create(commands, YDS_GUILD_ID);
=======
      dpp::slashcommand pm("pm", "Recieve a message from me.", bot.me.id);
      dpp::slashcommand pfp("pfp", "Get someone's profile picture.", bot.me.id);
      dpp::slashcommand msgsGet("msgs-get", "Get Messages", bot.me.id);

      constexpr int64_t min_val{1};
      constexpr int64_t max_val{100};

      msgsGet.add_option(
          dpp::command_option(dpp::co_integer, "quantity",
                              "Quantity of messages to get. Max - 100.")
              .set_min_value(min_val)
              .set_max_value(max_val));

      pfp.add_option(dpp::command_option(
          std::string, "user",
          "The user of the profile picture I should grab."));
      // bot.global_bulk_command_create({whoami, ping, pm, file});
      bot.guild_bulk_command_create({whoami, ping, pm, pfp, msgsGet},
                                    YDS_GUILD_ID);
>>>>>>> a4274b0421ce1a8fafc12dfeb4f49a5f2f7fe886
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
