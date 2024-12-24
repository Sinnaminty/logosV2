#include <dpp/appcommand.h>
#include <dpp/cache.h>
#include <dpp/channel.h>
#include <dpp/dpp.h>
#include <dpp/message.h>
#include <dpp/misc-enum.h>
#include <dpp/queues.h>

#include <dpp/restresults.h>
#include <dpp/snowflake.h>
#include <dpp/utility.h>
#include <out123.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <dpp/nlohmann/json.hpp>

#include <Logos/U.h>

using json = nlohmann::json;
using namespace U;

std::vector<std::vector<uint8_t>> pcmQueue;
std::string currentSong;

int main(int argc, const char* argv[]) {
  json configDocument;
  std::ifstream configFile("json/config.json");
  configFile >> configDocument;

  dpp::snowflake ydsGuildId(configDocument["yds-guild-id"]);
  dpp::snowflake watGuildId(configDocument["wat-guild-id"]);
  dpp::snowflake tvvGuildId(configDocument["tvv-guild-id"]);

  dpp::cluster bot(configDocument["token"], dpp::i_default_intents |
                                                dpp::i_guild_members |
                                                dpp::i_message_content);

  bot.on_log(dpp::utility::cout_logger());

  bot.on_voice_track_marker([&](const dpp::voice_track_marker_t& ev) {
    bot.log(dpp::loglevel::ll_debug, "on_voice_track_marker");

    auto v = ev.voice_client;

    /* If the voice channel was invalid, or there is an issue with it, then
     * tell the user. */
    if (!v || !v->is_ready()) {
      bot.log(dpp::loglevel::ll_error, "Connection Error. clearing queue..");
      currentSong.clear();
      pcmQueue.clear();
      return;
    }

    currentSong = ev.track_meta;

    if (pcmQueue.size() > 0) {
      std::vector<uint8_t> pcmData = pcmQueue.back();
      pcmQueue.pop_back();
      v->send_audio_raw((uint16_t*)pcmData.data(), pcmData.size());
      return;

    } else {
      bot.log(dpp::loglevel::ll_error, "Error dequeing song. clearing queue..");
      currentSong.clear();
      pcmQueue.clear();
      return;
    }
  });

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
            createEmbed(mType::GOOD, "⏯️ Currently playing: " + currentSong)));
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
            createEmbed(mType::GOOD, "⏯️ Currently playing: " + currentSong)));
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
            createEmbed(mType::BAD, "⚠️ I'm not in a voice channel, dork.")));
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
      event.thinking(false, [&](const dpp::confirmation_callback_t& callback) {

      });

      /* Get the voice channel the bot is in, in this current guild. */
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      /* If the voice channel was invalid, or there is an issue with it, then
       * tell the user. */
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "🔇 There was an issue with getting the voice channel. "
                        "Make sure I'm in a voice channel! :(")));
        return;
      }

      const std::string link =
          std::get<std::string>(event.get_parameter("link"));
      const std::string fileName = downloadSong(link);
      const std::vector<uint8_t> pcmData = encodeSong("music/" + fileName);
      bot.log(dpp::loglevel::ll_debug,
              "past pcm data. size = " + std::to_string(pcmData.size()));
      pcmQueue.emplace_back(pcmData);

      bot.log(dpp::loglevel::ll_debug,
              "pcmQueue size = " + std::to_string(pcmQueue.size()));
      v->voiceclient->insert_marker(fileName);

      event.edit_original_response(
          dpp::message(event.command.channel_id,
                       createEmbed(mType::GOOD, "🔈 Enqueued: " + fileName)));

      return;

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "pause") {
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

      /* If the track is playing
       * pause. */
      bot.log(dpp::loglevel::ll_debug,
              "is_playing = " + std::to_string(v->voiceclient->is_playing()));
      if (!v->voiceclient->is_paused()) {
        v->voiceclient->pause_audio(true);
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "⏯️ Pausing track: " + currentSong)));
        return;

      } else {
        v->voiceclient->pause_audio(false);
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "⏯️ Unpausing track: " + currentSong)));
        return;
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "warfstatus") {
      event.reply(
          dpp::message(event.command.channel_id,
                       createEmbed(mType::BAD, "Not Yet Implemented!")));
      return;
      // bot.request("https://api.warframestat.us/pc/", dpp::m_get,
      //             [](const dpp::http_request_completion_t& cc) {
      //               // This callback is called when the HTTP request
      //               // completes. See documentation of
      //               // dpp::http_request_completion_t for information
      //               // on the fields in the parameter.
      //               std::cout << "I got reply: " << cc.body
      //                         << " with HTTP status code: " << cc.status
      //                         << "\n";
      //             },
      //             "", "application/json", {});
      // event.reply("check console!");

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "rizzmeup") {
      dpp::snowflake user = event.command.get_issuing_user().id;

      /* Send a message to the user set above. */
      bot.direct_message_create(
          user, dpp::message("rizz."),
          [&](const dpp::confirmation_callback_t& callback) {
            /* If the callback errors, we want to send a message telling the
             * author that something went wrong. */

            if (callback.is_error()) {
              /* Here, we want the error message to be different if the user
               * we're trying to send a message to is the command author. */

              bot.log(dpp::loglevel::ll_error,
                      callback.get_error().human_readable);

              event.reply(dpp::message(event.command.channel_id,
                                       createEmbed(mType::BAD,
                                                   "I couldn't rizz you up..."))
                              .set_flags(dpp::m_ephemeral));

              return;
            }
          });

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "roll") {
      try {
        std::string roll = std::get<std::string>(event.get_parameter("roll"));
        std::string resp;
        auto diceVec = parseDiceString(roll);
        for (Dice d : diceVec) {
          resp += "**" + std::to_string(d.m_num) + "d" +
                  std::to_string(d.m_side) + "**: " + d.roll() + "\n";
        }

        event.reply(dpp::message(event.command.channel_id,
                                 createEmbed(mType::GOOD, resp)));
        return;

      } catch (std::invalid_argument& e) {
        event.reply(dpp::message(event.command.channel_id,
                                 createEmbed(mType::BAD, e.what())));

        return;
      }

    } else if (event.command.get_command_name() == "say") {
      event.thinking(false, [&](const dpp::confirmation_callback_t& callback) {

      });
      bool dl = false;
      try {
        dl = std::get<bool>(event.get_parameter("dl"));
      } catch (const std::exception& e) {
        bot.log(dpp::loglevel::ll_error, e.what());
      }

      const std::string sayString =
          std::get<std::string>(event.get_parameter("text"));

      if (!dl) {
        /* Get the voice channel the bot is in, in this current guild. */
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
        /* If the voice channel was invalid, or there is an issue with it, then
         * tell the user. */
        if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
          event.edit_original_response(dpp::message(
              event.command.channel_id,
              createEmbed(
                  mType::BAD,
                  "🔇 There was an issue with getting the voice channel. "
                  "Make sure I'm in a voice channel! :(")));
          return;
        }

        try {
          const std::vector<uint8_t> pcmData = encodeSay(sayString);

          v->voiceclient->send_audio_raw((uint16_t*)pcmData.data(),
                                         pcmData.size());
          event.edit_original_response(
              dpp::message(event.command.channel_id,
                           createEmbed(mType::GOOD, "🔈 Speaking! ")));

          return;

        } catch (std::exception& e) {
          bot.log(dpp::loglevel::ll_error, e.what());
          event.edit_original_response(dpp::message(
              event.command.channel_id,
              createEmbed(mType::BAD, "Something went wrong! Soz;;")));
          return;
        }

        // if we are downloading...
      } else {
        try {
          dpp::message msg(event.command.channel_id, "");
          downloadSay(sayString);
          msg.add_file("say.mp3", dpp::utility::read_file("say.mp3"));
          event.edit_original_response(msg);
          return;

        } catch (std::exception& e) {
          event.edit_original_response(dpp::message(
              event.command.channel_id, createEmbed(mType::BAD, e.what())));

          return;
        }
      }

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

  bot.on_ready([&](const dpp::ready_t& event) -> void {
    if (dpp::run_once<struct clear_bot_commands>()) {
      // bot.global_bulk_command_delete();
      bot.guild_bulk_command_delete(ydsGuildId);
      bot.guild_bulk_command_delete(watGuildId);
      bot.guild_bulk_command_delete(tvvGuildId);
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      // radio commands
      dpp::slashcommand join("join", "Joins the user's vc.", bot.me.id);
      dpp::slashcommand queue("queue", "Show music queue.", bot.me.id);
      dpp::slashcommand np("np", "Show currently playing song.", bot.me.id);
      dpp::slashcommand skip("skip", "Skip to the next song in queue.",
                             bot.me.id);
      dpp::slashcommand pause("pause", "Toggle pause the music.", bot.me.id);
      dpp::slashcommand stop(
          "stop", "Stop playing, clear queue and leave voice channel.",
          bot.me.id);

      dpp::slashcommand play("play", "Play a song.", bot.me.id);
      play.add_option((dpp::command_option(dpp::co_string, "link",
                                           "The link to the song.", true)));

      // other commands
      dpp::slashcommand rizzmeup("rizzmeup", "delta sigma alpha.", bot.me.id);

      dpp::slashcommand roll(
          "roll", "Roll any specified number of x sided die.", bot.me.id);
      roll.add_option(dpp::command_option(
          dpp::co_string, "roll", "Roll String (ex. '1d20 2d10 3d6')", true));

      dpp::slashcommand say("say", "Make me speak.(Moon Base Alpha friendly.)",
                            bot.me.id);
      say.add_option((dpp::command_option(dpp::co_string, "text",
                                          "What I should say.", true)));
      say.add_option((dpp::command_option(
          dpp::co_boolean, "dl",
          "Download this file and send it to the channel?", false)));

      dpp::slashcommand warfstatus(
          "warfstatus", "Get World State Data from Warframe.", bot.me.id);

      //   dpp::slashcommand archive (
      //       "archive",
      //       "Saves every text channel in this server.",
      //       bot.me.id );

      const std::vector<dpp::slashcommand> commands = {
          join, queue,    np,   skip, pause,     stop,
          play, rizzmeup, roll, say,  warfstatus};
      bot.guild_bulk_command_create(commands, ydsGuildId);
      bot.guild_bulk_command_create(commands, watGuildId);
      bot.guild_bulk_command_create(commands, tvvGuildId);

      bot.log(dpp::loglevel::ll_info, "Bot Ready!!!");
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
