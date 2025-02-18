#include <Logos/Logos.h>
#include <Logos/Schedule.h>
#include <Logos/Vox.h>
#include <dpp/appcommand.h>
#include <dpp/message.h>
#include <dpp/misc-enum.h>
#include <dpp/snowflake.h>
#include <exception>
#include <stdexcept>
#include <variant>

using json = nlohmann::json;
using namespace Logos;

const int SCHEDULE_FREQUENCY = 60;

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
      event.thinking(false,
                     [&](const dpp::confirmation_callback_t& callback) {});

      const dpp::command_interaction cmdData =
          event.command.get_command_interaction();

      dpp::guild* g = dpp::find_guild(event.command.guild_id);

      // Attempt to connect to a voice channel
      if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "🔇 You don't seem to be on a voice channel! :(")));
        return;
      }

      /* Tell the user we joined their channel. */
      event.edit_original_response(
          dpp::message(event.command.channel_id,
                       createEmbed(mType::GOOD, "🔈 Connecting to voice...")));

      /* Get the voice channel the bot is in, in this current guild. */
      dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

      /* If the voice channel was invalid, or there is an issue with it,
       * then tell the user. */
      if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "🔇 There was an issue with getting the voice channel. "
                        "Make sure I'm in a voice channel! :(")));
        return;
      }

      auto subCommand = cmdData.options[0];

      if (subCommand.name == "play") {
        std::string link = "";
        for (auto& param : subCommand.options) {
          if (param.name == "link" &&
              std::holds_alternative<std::string>(param.value)) {
            link = std::get<std::string>(param.value);
          }

          const std::string fileName = downloadSong(link);
          const std::vector<uint8_t> pcmData = encodeAudio("music/" + fileName);
          Carbon::getInstance().s_songQueue.emplace_back(pcmData);

          v->voiceclient->insert_marker(fileName);

          event.edit_original_response(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD, "🔈 Enqueued: " + fileName)));

          return;
        }

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "pause") {
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
        if (v && v->voiceclient && v->voiceclient->is_ready()) {
          event.reply(
              dpp::message(event.command.channel_id,
                           createEmbed(mType::GOOD, "⏯️ Leaving voice... ")));

          event.from->disconnect_voice(event.command.guild_id);
          return;
        }

        ///////////////////////////////////////////////////////////////////////////

      } else if (subCommand.name == "skip") {
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

      ///////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "vox") {
      const dpp::command_interaction cmdData =
          event.command.get_command_interaction();

      event.thinking(false,
                     [&](const dpp::confirmation_callback_t& callback) {});

      // holds the name of the subcommand
      auto subCommand = cmdData.options[0];

      if (subCommand.name == "speak") {
        std::string sayString = "";
        for (auto& param : subCommand.options) {
          if (param.name == "text" &&
              std::holds_alternative<std::string>(param.value)) {
            sayString = std::get<std::string>(param.value);
          }
        }
        dpp::guild* g = dpp::find_guild(event.command.guild_id);

        // Attempt to connect to a voice channel
        if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
          event.edit_original_response(dpp::message(
              event.command.channel_id,
              createEmbed(mType::BAD,
                          "🔇 You don't seem to be on a voice channel! :(")));
          return;
        }

        /* Tell the user we joined their channel. */
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "🔈 Connecting to voice...")));

        /* Get the voice channel the bot is in, in this current guild. */
        dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);
        // BUG: If not connected to voice chat initially, you need to run the
        // command twice.

        if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
          event.edit_original_response(dpp::message(
              event.command.channel_id,
              createEmbed(
                  mType::BAD,
                  "🔇 There was an issue with getting the voice channel. "
                  "Make sure I'm in a voice channel! :(")));
          return;
        }

        // if we're here, that means we're in vc and we ready
        try {
          const std::vector<uint8_t> pcmData = Vox::encodeSpeak(sayString);

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

      } else if (subCommand.name == "listen") {
        // listen
        event.edit_original_response(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD, "Currently being refactored, soz!")));
        return;

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

        auto members = event.command.get_guild().members;
        auto vcMembers = event.command.get_guild().voice_members;
        if (!Carbon::getInstance().s_recording) {
          try {
            for (auto [s, v] : vcMembers) {
              std::string username =
                  members.find(s)->second.get_user()->username;
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
              std::string username =
                  members.find(s)->second.get_user()->username;
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

      } else if (subCommand.name == "download") {
        if (!subCommand.options.empty()) {
          std::string sayString = "";
          for (auto& param : subCommand.options) {
            if (param.name == "text" &&
                std::holds_alternative<std::string>(param.value)) {
              sayString = std::get<std::string>(param.value);
            }
          }

          try {
            dpp::message msg(event.command.channel_id, "");
            Vox::downloadSpeak(sayString);
            msg.add_file("say.mp3", dpp::utility::read_file("say.mp3"));
            event.edit_original_response(msg);
            return;

          } catch (std::exception& e) {
            event.edit_original_response(dpp::message(
                event.command.channel_id, createEmbed(mType::BAD, e.what())));
            return;
          }
        }
      }

      ////////////////////////////////////////////////////////////////////////////////////////////

    } else if (event.command.get_command_name() == "schedule") {
      const dpp::command_interaction cmdData =
          event.command.get_command_interaction();

      // holds the name of the subcommand
      auto subCommand = cmdData.options[0];

      const dpp::snowflake userSnowflake = event.command.get_issuing_user().id;
      auto userSchedule = Schedule::getUserSchedule(userSnowflake);

      /** time to disqualify some dorks.
       * if ur timezone is NOT set, and ur not calling set_timezone, fuck you.
       * if u have no args and your command is NOT show, fuck you.
       * ^ this is incredibly pedantic and pointless but so am i so ha
       */
      if (userSchedule.m_timezone == "" && subCommand.name != "set_timezone") {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(
                mType::BAD,
                "Set your timezone first with /schedule set_timezone!\n")));
        return;
      }

      if (subCommand.options.empty() && subCommand.name != "show") {
        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::BAD,
                        "Empty args. how the FUCK did you do that???\n")));
        return;
      }

      try {
        if (subCommand.name == "set_timezone") {
          for (auto& param : subCommand.options) {
            if (param.name == "timezone" &&
                std::holds_alternative<std::string>(param.value)) {
              userSchedule.setTimezone(std::get<std::string>(param.value));

              event.reply(dpp::message(
                  event.command.channel_id,
                  createEmbed(mType::GOOD,
                              "Timezone set!\n" + userSchedule.toString())));
            }
          }

        } else if (subCommand.name == "show") {
          event.reply(
              dpp::message(event.command.channel_id,
                           createEmbed(mType::GOOD, userSchedule.toString())));

        } else if (subCommand.name == "add") {
          std::string name = "";
          std::string date = "";
          std::string time = "";

          for (auto& param : subCommand.options) {
            if (param.name == "name" &&
                std::holds_alternative<std::string>(param.value)) {
              name = std::get<std::string>(param.value);

            } else if (param.name == "date" &&
                       std::holds_alternative<std::string>(param.value)) {
              date = std::get<std::string>(param.value);

            } else if (param.name == "time" &&
                       std::holds_alternative<std::string>(param.value)) {
              time = std::get<std::string>(param.value);
            }
          }

          userSchedule.addEvent(name, date, time);

          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD,
                          "Event Added!\n" + userSchedule.toString())));

        } else if (subCommand.name == "edit") {
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

          userSchedule.editEvent(index, newName, newDate, newTime);
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::GOOD,
                          "Event Edited!\n" + userSchedule.toString())));

        } else if (subCommand.name == "remove") {
          if (!subCommand.options.empty()) {
            int index = 0;

            for (auto& param : subCommand.options) {
              if (param.name == "index" &&
                  std::holds_alternative<int64_t>(param.value)) {
                index = std::get<int64_t>(param.value);
              }
            }

            userSchedule.removeEvent(index);
            event.reply(dpp::message(
                event.command.channel_id,
                createEmbed(mType::GOOD,
                            "Event Deleted!\n" + userSchedule.toString())));
          }
        }

        /** the biggest catch of the century */
      } catch (const std::exception& e) {
        event.reply(dpp::message(event.command.channel_id,
                                 createEmbed(mType::BAD, e.what())));
      }
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
      // bot.global_bulk_command_delete();
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

      schedule.add_option(
          // For timezone.
          dpp::command_option(dpp::co_sub_command, "set_timezone",
                              "Set your timezone.")
              .add_option(dpp::command_option(dpp::co_string, "timezone",
                                              "What your timezone is.", true)

                              // North America
                              .add_choice(dpp::command_option_choice(
                                  "ET", std::string("America/New_York")))
                              .add_choice(dpp::command_option_choice(
                                  "CT", std::string("America/Chicago")))
                              .add_choice(dpp::command_option_choice(
                                  "MT", std::string("America/Denver")))
                              .add_choice(dpp::command_option_choice(
                                  "PT", std::string("America/Los_Angeles")))
                              .add_choice(dpp::command_option_choice(
                                  "Alaska", std::string("America/Anchorage")))
                              .add_choice(dpp::command_option_choice(
                                  "Hawaii", std::string("Pacific/Honolulu")))

                              // Europe
                              .add_choice(dpp::command_option_choice(
                                  "CET", std::string("Europe/Berlin")))
                              .add_choice(dpp::command_option_choice(
                                  "GMT", std::string("Europe/London")))
                              .add_choice(dpp::command_option_choice(
                                  "EET", std::string("Europe/Athens")))

                              // Asia
                              .add_choice(dpp::command_option_choice(
                                  "IST", std::string("Asia/Kolkata")))
                              .add_choice(dpp::command_option_choice(
                                  "CST", std::string("Asia/Shanghai")))
                              .add_choice(dpp::command_option_choice(
                                  "JST", std::string("Asia/Tokyo")))

                              // Oceania
                              .add_choice(dpp::command_option_choice(
                                  "AET", std::string("Australia/Sydney")))
                              .add_choice(dpp::command_option_choice(
                                  "NZT", std::string("Pacific/Auckland")))

                              // Africa
                              .add_choice(dpp::command_option_choice(
                                  "SAST", std::string("Africa/Johannesburg")))
                              .add_choice(dpp::command_option_choice(
                                  "WAT", std::string("Africa/Lagos")))

                              // South America
                              .add_choice(dpp::command_option_choice(
                                  "BRT", std::string("America/Sao_Paulo")))
                              .add_choice(dpp::command_option_choice(
                                  "ART", std::string("America/Buenos_Aires")))

                              ));

      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// Vox commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand vox("vox", "For VC Based Commands.", bot.me.id);

      vox.add_option(
          // For speak.
          dpp::command_option(dpp::co_sub_command, "speak",
                              "Speak in VC (Moonbase Alpha dectalk).")

              .add_option(dpp::command_option(dpp::co_string, "text",
                                              "What you want me to say.", true)

                              ));

      vox.add_option(
          // For listen.
          dpp::command_option(dpp::co_sub_command, "listen",
                              "Transcribes people in this VC.")

      );

      vox.add_option(
          // For download.
          dpp::command_option(
              dpp::co_sub_command, "download",
              "Sends a voice clip of my speech of your choosing.")
              .add_option(dpp::command_option(dpp::co_string, "text",
                                              "What you want me to say.", true))

      );

      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// other commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand roll(
          "roll", "Roll any specified number of x sided die.", bot.me.id);
      roll.add_option(dpp::command_option(
          dpp::co_string, "roll", "Roll String (ex. '1d20 2d10 3d6')", true));

      /////////////////////////////////////////////////////////////////////////////////////////////////

      const std::vector<dpp::slashcommand> commands = {radio, roll, vox,
                                                       schedule};

      // bot.global_bulk_command_create(commands);
      bot.guild_bulk_command_create(commands, ydsGuild);
      bot.log(dpp::loglevel::ll_info, "Bot Ready!!!");
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
