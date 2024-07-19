#include "info.h"
#include <dpp/colors.h>
#include <dpp/dpp.h>
#include <dpp/message.h>

int main() {
  dpp::cluster bot(BOT_TOKEN);

  bot.on_log(dpp::utility::cout_logger());

  /* The event is fired when someone issues your commands */
  bot.on_slashcommand([&bot](const dpp::slashcommand_t &event) {
    /* Check which command they ran */
    if (event.command.get_command_name() == "whoami") {
      /* Create an embed */
      dpp::embed embed =
          dpp::embed()
              .set_color(dpp::colors::pastel_light_blue)
              .set_title("Logos")
              .set_url("https://fizzysylv.xyz/")
              .set_author("Fizzy", "https://fizzysylv.xyz/",
                          "https://fizzysylv.xyz/images/fizzysylv.png")
              .set_description("Repitition Legitimizes.")
              .set_thumbnail("https://fizzysylv.xyz/images/fizzysylv.png")
              .add_field(" field title", "Some value here")
              .add_field("Inline field title", "Some value here", true)
              .add_field("Inline field title", "Some value here", true)
              .set_image("")
              .set_footer(dpp::embed_footer().set_text("meow~").set_icon(
                  "https://fizzysylv.xyz/images/fizzysylv.png"))
              .set_timestamp(time(0));

      /* Create a message with the content as our new embed. */
      dpp::message msg(event.command.channel_id, embed);

      /* Reply to the user with the message, containing our embed. */
      event.reply(msg);
    }
  });
  bot.on_ready([&bot](const dpp::ready_t &event) {
    if (dpp::run_once<struct clear_bot_commands>()) {
      bot.global_bulk_command_delete();
      bot.guild_bulk_command_delete(YDS_GUILD_ID);
    }
    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand whoami("whoami", "Information about logos.", bot.me.id);

      bot.global_bulk_command_create({whoami});
      bot.guild_bulk_command_create({whoami}, YDS_GUILD_ID);
    }
  });

  bot.start(dpp::st_wait);
}
