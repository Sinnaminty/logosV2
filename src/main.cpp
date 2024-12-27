// #include <liboai.h>
#include <Logos/Logos.h>
#include <dpp/cache.h>
#include <dpp/dispatcher.h>
#include <dpp/message.h>
#include <dpp/restresults.h>
#include <dpp/snowflake.h>

using json = nlohmann::json;
using namespace Logos;

int main(int argc, const char* argv[]) {
  json configDocument;
  json sDocument;

  std::ifstream configFile("json/config.json");
  std::ifstream s("json/s.json");

  configFile >> configDocument;
  s >> sDocument;

  dpp::snowflake ydsGuild(configDocument["yds-guild-id"]);

  dpp::cluster bot(sDocument["test-bot-token"], dpp::i_default_intents |
                                                    dpp::i_guild_members |
                                                    dpp::i_message_content);

  bot.on_log(dpp::utility::cout_logger());

  bot.on_voice_track_marker([&](const dpp::voice_track_marker_t& ev) {
    bot.log(dpp::loglevel::ll_debug, "on_voice_track_marker. currentSong = " +
                                         Carbon::getInstance().s_currentSong);

    auto v = ev.voice_client;

    /* If the voice channel was invalid, or there is an issue with it, then
     * tell the user. */
    if (!v || !v->is_ready()) {
      bot.log(dpp::loglevel::ll_error, "Connection Error. clearing queue..");
      Carbon::getInstance().s_currentSong = "";
      Carbon::getInstance().s_songQueue.clear();
      return;
    }

    Carbon::getInstance().s_currentSong = ev.track_meta;

    if (Carbon::getInstance().s_songQueue.size() > 0) {
      std::vector<uint8_t> pcmData = Carbon::getInstance().s_songQueue.back();
      Carbon::getInstance().s_songQueue.pop_back();
      v->send_audio_raw((uint16_t*)pcmData.data(), pcmData.size());
      return;

    } else {
      bot.log(dpp::loglevel::ll_error, "Error dequeing song. clearing queue..");
      Carbon::getInstance().s_currentSong = "";
      Carbon::getInstance().s_songQueue.clear();
      return;
    }
  });

  bot.on_voice_receive([&](const dpp::voice_receive_t& event) {
    if (Carbon::getInstance().s_recording) {
      if (Carbon::getInstance().s_userFileMap.find(event.user_id) !=
          Carbon::getInstance().s_userFileMap.end()) {
        Carbon::getInstance().s_userFileMap[event.user_id].write(
            (char*)event.audio, event.audio_size);
      }
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
            createEmbed(mType::GOOD, "⏯️ Currently playing: " +
                                         Carbon::getInstance().s_currentSong)));
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
          v->voiceclient->get_tracks_remaining() > 0) {
        v->voiceclient->skip_to_next_marker();
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "⏯️  Now playing: " +
                                         Carbon::getInstance().s_currentSong)));

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
      Carbon::getInstance().s_songQueue.emplace_back(pcmData);

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
            createEmbed(mType::GOOD, "⏯️ Pausing track: " +
                                         Carbon::getInstance().s_currentSong)));
        return;

      } else {
        v->voiceclient->pause_audio(false);
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "⏯️ Unpausing track: " +
                                         Carbon::getInstance().s_currentSong)));
        return;
      }

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

    } else if (event.command.get_command_name() == "transcribe") {
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

      auto members = event.command.get_guild().members;
      auto vcMembers = event.command.get_guild().voice_members;
      if (!Carbon::getInstance().s_recording) {
        try {
          for (auto [s, v] : vcMembers) {
            std::string username = members.find(s)->second.get_user()->username;
            std::string filename = "./" + username + ".pcm";
            Carbon::getInstance().s_userFileMap[s] =
                std::ofstream(filename, std::ios::binary);
          }
          Carbon::getInstance().s_recording = true;

          event.edit_original_response(
              dpp::message(event.command.channel_id,
                           createEmbed(mType::GOOD, "Started Recording!")));
          return;

        } catch (const std::exception& e) {
          event.edit_original_response(dpp::message(
              event.command.channel_id, createEmbed(mType::BAD, e.what())));
          return;
        }

      } else {
        try {
          Carbon::getInstance().s_recording = false;
          dpp::message msg(event.command.channel_id,
                           createEmbed(mType::GOOD, "Stopped Recording!"));
          for (auto& [s, o] : Carbon::getInstance().s_userFileMap) {
            std::string username = members.find(s)->second.get_user()->username;
            std::string filename = "./" + username + ".pcm";
            o.close();
            msg.add_file(filename, dpp::utility::read_file(filename));
          }

          Carbon::getInstance().s_userFileMap.clear();
          event.edit_original_response(msg);
          return;

        } catch (const std::exception& e) {
          event.edit_original_response(dpp::message(
              event.command.channel_id, createEmbed(mType::BAD, e.what())));

          return;
        }
      }

    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    else if (event.command.get_command_name() == "clearLocalCommands") {
      bot.guild_bulk_command_delete(
          event.command.get_guild().id,
          [&](const dpp::confirmation_callback_t& callback) {
            if (callback.is_error()) {
              event.reply(dpp::message(
                  event.command.channel_id,
                  createEmbed(mType::BAD,
                              callback.get_error().human_readable)));

            } else {
              event.reply(dpp::message(
                  event.command.channel_id,
                  createEmbed(
                      mType::GOOD,
                      "Deleted commands for Guild: " +
                          dpp::find_guild(event.command.guild_id)->name)));
            }
          });
    }
  });

  ////////////////////////////////////////////////////////////////////////////////////////////

  bot.on_ready([&](const dpp::ready_t& event) -> void {
    if (dpp::run_once<struct clear_bot_commands>()) {
      bot.guild_bulk_command_delete(ydsGuild);
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// radio commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand join("join", "Joins the user's vc.", bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand queue("queue", "Show music queue.", bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand np("np", "Show currently playing song.", bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand skip("skip", "Skip to the next song in queue.",
                             bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand pause("pause", "Toggle pause the music.", bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand stop(
          "stop", "Stop playing, clear queue and leave voice channel.",
          bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand play("play", "Play a song.", bot.me.id);
      play.add_option((dpp::command_option(dpp::co_string, "link",
                                           "The link to the song.", true)));

      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// other commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand roll(
          "roll", "Roll any specified number of x sided die.", bot.me.id);
      roll.add_option(dpp::command_option(
          dpp::co_string, "roll", "Roll String (ex. '1d20 2d10 3d6')", true));

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand say("say", "Make me speak.(Moon Base Alpha friendly.)",
                            bot.me.id);
      say.add_option((dpp::command_option(dpp::co_string, "text",
                                          "What I should say.", true)));
      say.add_option((dpp::command_option(
          dpp::co_boolean, "dl",
          "Download this file and send it to the channel?", false)));

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand transcribe("transcribe", "Record VC.", bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand clearLocalCommands(
          "clearLocalCommands", "Clears local commands in this guild",
          bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      const std::vector<dpp::slashcommand> commands = {join,
                                                       queue,
                                                       np,
                                                       skip,
                                                       pause,
                                                       stop,
                                                       play,
                                                       roll,
                                                       say,
                                                       transcribe,
                                                       clearLocalCommands};

      bot.guild_bulk_command_create(commands, ydsGuild);

      bot.log(dpp::loglevel::ll_info, "Bot Ready!!!");
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
