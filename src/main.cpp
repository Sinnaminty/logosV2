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
            createEmbed(mType::BAD, "⚠️ The queue is empty, fool")));
        return;

      } else {
        event.reply(dpp::message(event.command.channel_id,
                                 createEmbed(mType::GOOD, resp)));
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "np") {
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready() &&
          v->voiceclient->get_tracks_remaining() > 0) {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "⏯️ Currently playing: " + current_song)));
        return;

      } else {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "...No sounds here except crickets... 🦗")));
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "skip") {
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready() &&
          v->voiceclient->get_tracks_remaining() > 1) {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "⏯️ Currently playing: " + current_song)));
        v->voiceclient->skip_to_next_marker();
        return;

      } else {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD, "⚠️No song to skip... I have nothing!")));
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "stop") {
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      if (v && v->voiceclient && v->voiceclient->is_ready()) {
        event.reply(
            dpp::message(event.command.channel_id,
                         createEmbed(mType::GOOD, "⏯️ Leaving voice... ")));

        event.from->disconnect_voice(event.command.guild_id);
        return;

      } else {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD, "⚠️I'm not in a voice channel, dork.")));
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "join") {
      dpp::guild* g = dpp::find_guild(event.command.guild_id);

      /* Attempt to connect to a voice channel, returns false if we fail to
       * connect. */
      if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "🔇 You don't seem to be on a voice channel! :(")));
        return;
      }

      /* Tell the user we joined their channel. */
      event.reply(
          dpp::message(event.command.channel_id,
                       createEmbed(mType::GOOD, "🔈 Connecting to voice...")));

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "play") {
      /* Get the voice channel the bot is in, in this current guild. */
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      /* If the voice channel was invalid, or there is an issue with it, then
       * tell the user. */
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "🔇 There was an issue with getting the voice channel. "
                        "Make sure I'm in a voice channel! :(")));
        return;
      }

      /* Stream the already decoded MP3 file. This passes the PCM data to the
       * library to be encoded to OPUS */
      std::vector<uint8_t> pcmdata = encodeSong("music/a.mp3");

      v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());

      event.reply(dpp::message(event.command.channel_id,
                               createEmbed(mType::GOOD, "🔈 Playing!")));
      return;

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
      dpp::slashcommand join("join", "Joins the user's vc.", bot.me.id);
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

      dpp::slashcommand warfstatus(
          "warfstatus", "Get World State Data from Warframe.", bot.me.id);

      //   dpp::slashcommand archive (
      //       "archive",
      //       "Saves every text channel in this server.",
      //       bot.me.id );

      const std::vector<dpp::slashcommand> commands = {
          join, queue, np, skip, stop, play, warfstatus};
      bot.guild_bulk_command_create(commands, ydsGuildId);
      std::cout << "Bot Ready!\n";
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
