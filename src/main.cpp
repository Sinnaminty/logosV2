#include <Logos/Logos.h>
#include <Logos/Schedule.h>
#include <dpp/appcommand.h>
#include <dpp/message.h>
#include <dpp/misc-enum.h>
#include <dpp/snowflake.h>
#include <exception>
#include <stdexcept>
#include <variant>

using json = nlohmann::json;
using namespace Logos;

const int SCHEDULE_FREQUENCY = 5;

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
    if (event.command.get_command_name() == "radio") {
      event.reply(dpp::message(
          event.command.channel_id,
          createEmbed(mType::BAD, "Currently being refactored, soz!")));
      return;
      // god this sucks..
      const dpp::command_interaction cmdData =
          event.command.get_command_interaction();

      auto subCommand = cmdData.options[0];
      if (subCommand.name == "play") {
        if (!subCommand.options.empty()) {
          std::string link = "";
          for (auto& param : subCommand.options) {
            if (param.name == "link" &&
                std::holds_alternative<std::string>(param.value)) {
              link = std::get<std::string>(param.value);
            }
          }
          event.thinking(false,
                         [&](const dpp::confirmation_callback_t& callback) {

                         });

          /* Get the voice channel the bot is in, in this current guild. */
          dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

          /* If the voice channel was invalid, or there is an issue with it,
           * then tell the user. */
          if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
            event.edit_original_response(dpp::message(
                event.command.channel_id,
                createEmbed(
                    mType::BAD,
                    "🔇 There was an issue with getting the voice channel. "
                    "Make sure I'm in a voice channel! :(")));
            return;
          }

          const std::string fileName = downloadSong(link);
          const std::vector<uint8_t> pcmData = encodeSong("music/" + fileName);
          Carbon::getInstance().s_songQueue.emplace_back(pcmData);

          v->voiceclient->insert_marker(fileName);

          event.edit_original_response(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD, "🔈 Enqueued: " + fileName)));

          return;
        }

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "pause") {
        /* Get the voice channel the bot is in, in this current guild. */
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

        /* If the voice channel was invalid, or there is an issue with it, then
         * tell the user. */
        if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(
                  mType::BAD,
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
              createEmbed(
                  mType::GOOD,
                  "⏯️ Pausing track: " + Carbon::getInstance().s_currentSong)));
          return;

        } else {
          v->voiceclient->pause_audio(false);
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD,
                          "⏯️ Unpausing track: " +
                              Carbon::getInstance().s_currentSong)));
          return;
        }

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "stop") {
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

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "skip") {
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

        if (v && v->voiceclient && v->voiceclient->is_ready() &&
            v->voiceclient->get_tracks_remaining() > 0) {
          v->voiceclient->skip_to_next_marker();
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(
                  mType::GOOD,
                  "⏯️  Now playing: " + Carbon::getInstance().s_currentSong)));

          return;

        } else {
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::BAD, "⚠️No song to skip... I have nothing!")));
          return;
        }

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "queue") {
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

        } else {
          event.reply(dpp::message(event.command.channel_id,
                                   createEmbed(mType::GOOD, resp)));
        }

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "np") {
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

        if (v && v->voiceclient && v->voiceclient->is_ready() &&
            v->voiceclient->get_tracks_remaining() > 0) {
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD,
                          "⏯️ Currently playing: " +
                              Carbon::getInstance().s_currentSong)));

        } else {
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::BAD,
                          "...No sounds here except crickets... 🦗")));
        }
      }

      ///////////////////////////////////////////////////////////////////////////

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
      event.edit_original_response(
          dpp::message(event.command.channel_id,
                       createEmbed(mType::GOOD, "🔈 Connecting to voice...")));

      const std::string sayString =
          std::get<std::string>(event.get_parameter("text"));

      auto paramDl = event.get_parameter("dl");
      bool dl = false;
      if (std::holds_alternative<bool>(paramDl)) {
        dl = std::get<bool>(paramDl);
      }

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
      event.reply(dpp::message(
          event.command.channel_id,
          createEmbed(mType::BAD, "Currently being refactored, soz!")));
      return;
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

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "schedule") {
      const dpp::command_interaction cmdData =
          event.command.get_command_interaction();

      // holds the name of the subcommand
      auto subCommand = cmdData.options[0];

      const dpp::snowflake userSnowflake = event.command.get_issuing_user().id;

      if (subCommand.name == "show") {
        try {
          auto userSchedule = Schedule::getUserSchedule(userSnowflake);
          event.reply(
              dpp::message(event.command.channel_id,
                           createEmbed(mType::GOOD, userSchedule.toString())));

        } catch (const std::exception& e) {
          event.reply(dpp::message(event.command.channel_id,
                                   createEmbed(mType::BAD, e.what())));
        }

        return;

      } else if (subCommand.name == "add") {
        if (!subCommand.options.empty()) {
          std::string eventName = "";
          std::string eventDate = "";
          std::string eventTime = "";
          auto userSchedule = Schedule::getUserSchedule(userSnowflake);

          for (auto& param : subCommand.options) {
            if (param.name == "name" &&
                std::holds_alternative<std::string>(param.value)) {
              eventName = std::get<std::string>(param.value);

            } else if (param.name == "date" &&
                       std::holds_alternative<std::string>(param.value)) {
              eventDate = std::get<std::string>(param.value);

            } else if (param.name == "time" &&
                       std::holds_alternative<std::string>(param.value)) {
              eventTime = std::get<std::string>(param.value);
            }
          }

          try {
            userSchedule.addEvent(eventName, eventDate, eventTime);

            event.reply(dpp::message(
                event.command.channel_id,
                createEmbed(mType::GOOD,
                            "Event Added!\n" + userSchedule.toString())));

          } catch (const std::exception& e) {
            event.reply(dpp::message(event.command.channel_id,
                                     createEmbed(mType::BAD, e.what())));
          }
        }

      } else if (subCommand.name == "edit") {
        // gonna do remove first
        if (!subCommand.options.empty()) {
          int index = 0;
          std::string newName = "";
          std::string newDate = "";
          std::string newTime = "";
          for (auto& param : subCommand.options) {
            if (param.name == "index" &&
                std::holds_alternative<int64_t>(param.value)) {
              index = std::get<int64_t>(param.value);

            } else if (param.name == "name" &&
                       std::holds_alternative<std::string>(param.value)) {
              newName = std::get<std::string>(param.value);

            } else if (param.name == "date" &&
                       std::holds_alternative<std::string>(param.value)) {
              newDate = std::get<std::string>(param.value);

            } else if (param.name == "time" &&
                       std::holds_alternative<std::string>(param.value)) {
              newTime = std::get<std::string>(param.value);
            }
          }

          auto userSchedule = Schedule::getUserSchedule(userSnowflake);
          try {
            userSchedule.editEvent(index, newName, newDate, newTime);

          } catch (const std::exception& e) {
            event.reply(dpp::message(event.command.channel_id,
                                     createEmbed(mType::BAD, e.what())));
          }

          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD,
                          "Event Edited!\n" + userSchedule.toString())));
        }

      } else if (subCommand.name == "remove") {
        if (!subCommand.options.empty()) {
          int index = 0;
          auto userSchedule = Schedule::getUserSchedule(userSnowflake);

          for (auto& param : subCommand.options) {
            if (param.name == "index" &&
                std::holds_alternative<int64_t>(param.value)) {
              index = std::get<int64_t>(param.value);
            }
          }

          try {
            userSchedule.removeEvent(index);
          } catch (const std::exception& e) {
            event.reply(dpp::message(event.command.channel_id,
                                     createEmbed(mType::BAD, e.what())));
          }
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD,
                          "Event Deleted!\n" + userSchedule.toString())));
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "clear") {
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
    bot.start_timer(
        [&](const dpp::timer& timer) {
          try {
            auto schEvent = Schedule::checkGlobalSchedule();
            if (schEvent == std::nullopt) {
              bot.log(dpp::ll_debug,
                      "checkGlobalSchedule: schEvent is nullopt!");
              return;
            }

            // remember that checkGlobalSchedule will remove an event if
            // it's present. this may not work.
            auto userSchedule = schEvent->first;
            auto userScheduleEntry = schEvent->second;
            const auto userSnowflake = dpp::snowflake(userSchedule.m_snowflake);
            bot.direct_message_create(
                userSnowflake,
                createEmbed(mType::EVENT, userScheduleEntry.toString()));

          } catch (const std::runtime_error& e) {
            bot.log(dpp::ll_error, e.what());
            return;
          }
        },
        SCHEDULE_FREQUENCY);

    if (dpp::run_once<struct clear_bot_commands>()) {
      bot.guild_bulk_command_delete(ydsGuild);
    }

    if (dpp::run_once<struct register_bot_commands>()) {
      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// radio commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand radio("radio", "Interact with the Radio.", bot.me.id);

      radio.add_option(
          // For play.
          dpp::command_option(dpp::co_sub_command, "play", "Play a song.")
              .add_option(dpp::command_option(
                  dpp::co_string, "link", "Youtube link to the song.", true))

      );

      radio.add_option(
          // For pause.
          dpp::command_option(dpp::co_sub_command, "pause",
                              "Pause/Plays the current track.")

      );

      radio.add_option(
          // For stop.
          dpp::command_option(
              dpp::co_sub_command, "stop",
              "Stops the music, clears the queue and leaves vc.")

      );

      radio.add_option(
          // For skip.
          dpp::command_option(dpp::co_sub_command, "skip",
                              "Skips the currently playing song.")

      );

      radio.add_option(
          // For queue.
          dpp::command_option(dpp::co_sub_command, "queue",
                              "Shows the current music queue.")

      );

      radio.add_option(
          // For now playing.
          dpp::command_option(dpp::co_sub_command, "np",
                              "Shows the current playing song.")

      );

      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// Schedule commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand schedule("schedule", "Show schedule.", bot.me.id);

      schedule.add_option(
          // For show.
          dpp::command_option(dpp::co_sub_command, "show",
                              "Show your schedule.")

      );

      schedule.add_option(
          // For add.
          dpp::command_option(dpp::co_sub_command, "add",
                              "Add an event to your Schedule.")
              .add_option(dpp::command_option(dpp::co_string, "name",
                                              "Name of the Event.", true))
              .add_option(dpp::command_option(dpp::co_string, "date",
                                              "Date of the Event.", true))
              .add_option(dpp::command_option(dpp::co_string, "time",
                                              "Time of the Event.", true))

      );

      schedule.add_option(
          // For edit.
          dpp::command_option(dpp::co_sub_command, "edit",
                              "Edit your schedule.")

              .add_option(dpp::command_option(dpp::co_integer, "index",
                                              "Index for the event to edit.",
                                              true))

              .add_option(dpp::command_option(dpp::co_string, "name",
                                              "New event name.", false))

              .add_option(dpp::command_option(dpp::co_string, "date",
                                              "New date.", false))

              .add_option(dpp::command_option(dpp::co_string, "time",
                                              "New time.", false))

      );

      schedule.add_option(
          // For remove.
          dpp::command_option(dpp::co_sub_command, "remove",
                              "Remove an event from your schedule.")

              .add_option(dpp::command_option(dpp::co_integer, "index",
                                              "Index for the event to remove.",
                                              true))

      );

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

      dpp::slashcommand clear("clear", "Clears local commands in this guild",
                              bot.me.id);

      /////////////////////////////////////////////////////////////////////////////////////////////////

      const std::vector<dpp::slashcommand> commands = {
          radio, roll, say, transcribe, schedule, clear};

      bot.guild_bulk_command_create(commands, ydsGuild);

      bot.log(dpp::loglevel::ll_info, "Bot Ready!!!");
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
