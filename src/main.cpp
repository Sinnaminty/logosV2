#include <dpp/cache.h>
#include <dpp/channel.h>
#include <dpp/dpp.h>
#include <dpp/message.h>
#include <dpp/queues.h>

#include <out123.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <vector>

#include <dpp/nlohmann/json.hpp>

#include <Logos/U.h>

using json = nlohmann::json;
using namespace U;

std::string current_song;

int main(int argc, const char* argv[]) {
  json configDocument;
  std::ifstream configFile("json/config.json");
  configFile >> configDocument;

  dpp::snowflake ydsGuildId(configDocument["yds-guild-id"]);
  dpp::snowflake bosGuildId(configDocument["bos-guild-id"]);

  dpp::cluster bot(configDocument["token"], dpp::i_default_intents |
                                                dpp::i_guild_members |
                                                dpp::i_message_content);

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand([&](const dpp::slashcommand_t& event) {
    if (event.command.get_command_name() == "queue") {
      std::string resp = "__**Current Queue:**__\n\n";
      bool any = false;

      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready()) {
        std::vector<std::string> songqueue =
            v->voiceclient->get_marker_metadata();
        for (auto& s : songqueue) {
          any = true;
          if (resp.length() < 2048) {
            resp += "🎵 " + s + "\n";
          } else {
            break;
          }
        }
      }

      if (!any) {
        event.reply(dpp::message(
            event.command.channel_id,
            U::createEmbed(U::mType::BAD, "⚠️ The queue is empty, fool")));
      } else {
        event.reply(dpp::message(event.command.channel_id,
                                 U::createEmbed(U::mType::GOOD, resp)));
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "np") {
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready() &&
          v->voiceclient->get_tracks_remaining() > 0) {
        event.reply(dpp::message(
            event.command.channel_id,
            U::createEmbed(U::mType::GOOD,
                           "⏯️ Currently playing: " + current_song)));
      } else {
        event.reply(dpp::message(
            event.command.channel_id,
            U::createEmbed(U::mType::BAD,
                           "...No sounds here except crickets... 🦗")));
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "skip") {
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready() &&
          v->voiceclient->get_tracks_remaining() > 1) {
        event.reply(dpp::message(
            event.command.channel_id,
            U::createEmbed(U::mType::GOOD,
                           "⏯️ Currently playing: " + current_song)));
        v->voiceclient->skip_to_next_marker();
      } else {
        event.reply(
            dpp::message(event.command.channel_id,
                         U::createEmbed(U::mType::BAD,
                                        "No song to skip... I have nothing!")));
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "stop") {
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready()) {
        event.reply(dpp::message(
            event.command.channel_id,
            U::createEmbed(U::mType::GOOD, "⏯️ Leaving voice... ")));
        event.from->disconnect_voice(event.command.guild_id);

      } else {
        event.reply(
            dpp::message(event.command.channel_id,
                         U::createEmbed(U::mType::BAD,
                                        "I'm not in a voice channel, dork.")));
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "play") {
      song_to_load = trim(std::get<std::string>(parameters[0].second));
      last_ch_id = src.channel_id;
      dpp::voiceconn* v = bot.get_shard(0)->get_voice(src.guild_id);
      if (v && v->voiceclient && v->voiceclient->is_ready()) {
        start_play(v->voiceclient, bot, &command_handler, &src);
      } else {
        dpp::guild* g = dpp::find_guild(src.guild_id);
        if (!g->connect_member_voice(src.issuer.id)) {
          bad_embed(command_handler, src,
                    "🔇 You don't seem to be on a voice channel! :(");
        } else {
          good_embed(command_handler, src, "🔈 Connecting to voice...");
        }
      }
      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "search") {
      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "warfstatus") {
      bot.request("https://api.warframestat.us/pc/", dpp::m_get,
                  [](const dpp::http_request_completion_t& cc) {
                    // This callback is called when the HTTP request
                    // completes. See documentation of
                    // dpp::http_request_completion_t for information
                    // on the fields in the parameter.
                    std::cout << "I got reply: " << cc.body
                              << " with HTTP status code: " << cc.status
                              << "\n";
                  },
                  "", "application/json", {});
      event.reply("check console!");

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "archive") {
      dpp::snowflake guild_id = event.command.guild_id;

      // Fetch all channels in the guild
      bot.channels_get(
          guild_id, [&](const dpp::confirmation_callback_t& callback) {
            if (callback.is_error()) {
              std::cout << callback.get_error().message << "\n";
              return;
            }

            auto channels = callback.get<dpp::channel_map>();

            std::cout << "got all channels!\n";

            // Process all channels within the server.
            for (const auto& [id, channel] : channels) {
              if (channel.is_text_channel()) {
                std::cout << "is channel!: " << channel.name << "\n";
                bot.messages_get(
                    id, 0, 0, 0, 100,
                    [&](const dpp::confirmation_callback_t& callback) {
                      if (callback.is_error()) {
                        std::cerr << "Failed to fetch messages "
                                     "for channel: "
                                  << channel.name << "\n";
                        return;
                      }

                      auto messages = callback.get<dpp::message_map>();

                      std::cout << "msgs get! channel:" << channel.name << "\n";

                      dpp::cache<dpp::message> cache;
                      for (const auto& [id, msg] : messages) {
                        dpp::message* m = new dpp::message;
                        *m = msg;
                        cache.store(m);
                      }
                      std::cout << "all messages saved for: " << channel.name
                                << ". passing to archiveChannel.";

                      U::archiveChannel(channel.name, cache);
                    });
              }
            }
          });
    }
  });

  bot.on_ready([&bot, &ydsGuildId](const dpp::ready_t& event) -> void {
    if (dpp::run_once<struct clear_bot_commands>()) {
      // bot.global_bulk_command_delete();
      bot.guild_bulk_command_delete(ydsGuildId);
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand queue("queue", "Show music queue.", bot.me.id);
      dpp::slashcommand np("np", "Show currently playing song.", bot.me.id);
      dpp::slashcommand skip("skip", "Skip to the next song in queue.",
                             bot.me.id);
      dpp::slashcommand stop(
          "stop", "Stop playing, clear queue and leave voice channel.",
          bot.me.id);
      dpp::slashcommand play("play", "Play a song.", bot.me.id);
      play.add_option((dpp::command_option(dpp::co_string, "name",
                                           "The name of the song.", true)));
      dpp::slashcommand search("search", "Search for a song.", bot.me.id);
      search.add_option((dpp::command_option(dpp::co_string, "name",
                                             "The name of the song.", true)));

      dpp::slashcommand whoami("whoami", "Information about Logos.", bot.me.id);

      dpp::slashcommand warfstatus(
          "warfstatus", "Get World State Data from Warframe.", bot.me.id);

      //   dpp::slashcommand archive (
      //       "archive",
      //       "Saves every text channel in this server.",
      //       bot.me.id );

      const std::vector<dpp::slashcommand> commands = {
          queue, np, skip, stop, play, search, whoami, warfstatus};
      bot.guild_bulk_command_create(commands, ydsGuildId);
      std::cout << "Bot Ready!\n";
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
