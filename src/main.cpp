#include <dpp/cache.h>
#include <dpp/channel.h>
#include <dpp/dpp.h>
#include <dpp/message.h>
#include <dpp/queues.h>

#include <mpg123.h>
#include <out123.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <vector>

#include <dpp/nlohmann/json.hpp>

#include <Logos/U.h>

using json = nlohmann::json;

std::string song_to_load = "";

dpp::snowflake last_ch_id = 0;
bool encode_thread_active = false;

std::vector<uint8_t> get_song(std::string file) {
  std::vector<uint8_t> pcmdata;

  mpg123_init();

  int err;
  mpg123_handle* mh = mpg123_new(NULL, &err);
  unsigned char* buffer;
  size_t buffer_size;
  size_t done;
  int channels, encoding;
  long rate;

  mpg123_param(mh, MPG123_FORCE_RATE, 48000, 48000.0);

  buffer_size = mpg123_outblock(mh);
  buffer = new unsigned char[buffer_size];

  mpg123_open(mh, file.c_str());
  mpg123_getformat(mh, &rate, &channels, &encoding);

  unsigned int counter = 0;
  for (int totalBtyes = 0;
       mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK;) {
    for (auto i = 0; i < buffer_size; i++) {
      pcmdata.push_back(buffer[i]);
    }
    counter += buffer_size;
    totalBtyes += done;
  }
  delete buffer;
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
  return pcmdata;
}

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
    if (event.command.get_command_name() == "whoami") {
      dpp::embed embed =
          U::createEmbed(U::mType::GOOD, "Repetition Legitimizes.");
      dpp::message msg(event.command.channel_id, embed);
      event.reply(msg);

    } else if (event.command.get_command_name() == "queue") {
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

    } else if (event.command.get_command_name() == "play") {
    } else if (event.command.get_command_name() == "search") {
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
