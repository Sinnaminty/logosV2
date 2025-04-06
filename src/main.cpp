#include <Logos/Logos.h>
#include <Logos/Oot.h>
#include <Logos/Schedule.h>
#include <Logos/Vox.h>
#include <dpp/appcommand.h>
#include <dpp/dpp.h>
#include <dpp/httpsclient.h>
#include <dpp/message.h>
#include <dpp/misc-enum.h>
#include <dpp/queues.h>
#include <dpp/snowflake.h>
#include <algorithm>
#include <cctype>
#include <exception>
#include <stdexcept>
#include <string>
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

  // dpp::cluster bot(sDocument["bot-token"], dpp::i_default_intents |
  //                                             dpp::i_guild_members |
  //                                            dpp::i_message_content);

  bot.on_log(dpp::utility::cout_logger());

  bot.on_slashcommand([&](const dpp::slashcommand_t& event) {
    /*
     * Vox
     */
    if (event.command.get_command_name() == "vox") {
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
        dpp::voiceconn* v = event.from()->get_voice(event.command.guild_id);
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
      /*
       * Schedule
       */
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

      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// oot commands
      /////////////////////////////////////////////////////////////////////////////////////////////////
    } else if (event.command.get_command_name() == "oot") {
      const dpp::command_interaction cmdData =
          event.command.get_command_interaction();
      auto subCommand = cmdData.options[0];

      if (subCommand.name == "add") {
        dpp::snowflake fileId;
        for (auto& param : subCommand.options) {
          if (param.name == "json") {
            fileId = std::get<dpp::snowflake>(param.value);
          }
        }

        auto it = event.command.resolved.attachments.find(fileId);
        if (it == event.command.resolved.attachments.end()) {
          event.reply(dpp::message(
              event.command.channel_id,
              createEmbed(mType::BAD, "No valid JSON attachment found, hoe.")));
          return;
        }

        dpp::attachment att = it->second;
        std::string fileUrl = att.url;

        bot.request(fileUrl, dpp::m_get,
                    [&](const dpp::http_request_completion_t& response) {
                      if (response.status == 200) {
                        bot.log(dpp::ll_info, "Downloaded JSON: ");
                        Oot::OOTItemHints::init(response.body);

                      } else {
                        bot.log(dpp::ll_error, "Request fail. Suffer.");
                      }
                    });

        event.reply(dpp::message(
            event.command.channel_id,
            createEmbed(mType::GOOD, "Item hints has been loaded!")));

      } else if (subCommand.name == "hint") {
        std::string itemName =
            std::get<std::string>(subCommand.options[0].value);

        if (itemName.empty()) {
          bot.log(dpp::ll_error, "itemName empty");
        }
        std::vector<std::string> locations =
            Oot::OOTItemHints::getItemLocations(itemName);

        std::string message = "**" + itemName + "** is at..\n";
        for (std::string location : locations) {
          message += location + "\n";
        }

        event.reply(dpp::message(event.command.channel_id,
                                 createEmbed(mType::GOOD, message)));
      } else if (subCommand.name == "hintloc") {
        std::string locationName =
            std::get<std::string>(subCommand.options[0].value);

        if (locationName.empty()) {
          bot.log(dpp::ll_error, "locationName empty");
        }
        std::string item = Oot::OOTItemHints::getItemFromLocation(locationName);

        std::string message =
            "**" + locationName + "** contains.. " + item + "\n";

        event.reply(dpp::message(event.command.channel_id,
                                 createEmbed(mType::GOOD, message)));
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

    // if (dpp::run_once<struct clear_bot_commands>()) {
    //   // bot.global_bulk_command_delete();
    //   bot.guild_bulk_command_delete(ydsGuild);
    // }

    if (dpp::run_once<struct register_bot_commands>()) {
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

              .add_option(dpp::command_option(
                  dpp::co_string, "text", "What you want me to say.", true)));

      vox.add_option(
          // For download.
          dpp::command_option(
              dpp::co_sub_command, "download",
              "Sends a voice clip of my speech of your choosing.")
              .add_option(dpp::command_option(
                  dpp::co_string, "text", "What you want me to say.", true)));

      /////////////////////////////////////////////////////////////////////////////////////////////////
      /// oot commands
      /////////////////////////////////////////////////////////////////////////////////////////////////

      dpp::slashcommand oot("oot", "For Ocarina of Time Based Commands.",
                            bot.me.id);

      oot.add_option(
          // For hint.
          dpp::command_option(dpp::co_sub_command, "hint",
                              "Ask for a hint for this rando run.")

              .add_option(dpp::command_option(dpp::co_string, "item",
                                              "What item do you want to hint?",
                                              true)
                              .set_auto_complete(true)));

      oot.add_option(
          // For hintloc.
          dpp::command_option(dpp::co_sub_command, "hintloc",
                              "Tells you what item is at this location.")

              .add_option(dpp::command_option(
                              dpp::co_string, "location",
                              "What location do you want to check?", true)
                              .set_auto_complete(true)));

      oot.add_option(
          // For add.
          dpp::command_option(dpp::co_sub_command, "add",
                              "Add a rando json to track.")

              .add_option(dpp::command_option(dpp::co_attachment, "json",
                                              "Select your json.", true)));

      const std::vector<dpp::slashcommand> commands = {vox, schedule, oot};

      // bot.global_bulk_command_create(commands);
      bot.guild_bulk_command_create(commands, ydsGuild);
      bot.log(dpp::loglevel::ll_info, "Bot Ready!!!");
    }
  });

  bot.on_autocomplete([&](const dpp::autocomplete_t& event) {
    //    bot.log(dpp::ll_info, "fuehh!");

    for (auto& opt : event.options) {
      //     bot.log(dpp::ll_info, opt.name);
      if (opt.name == "hint") {
        // bot.log(dpp::ll_info, opt.options[0].name);
        std::string uservalue = std::get<std::string>(opt.options[0].value);
        // bot.log(dpp::ll_info, "opt.value" + uservalue);
        dpp::interaction_response response(dpp::ir_autocomplete_reply);

        // Get all item names from OOTItemHints
        std::vector<std::string> all_items;
        for (const auto& pair : Oot::OOTItemHints::m_itemToLocation) {
          all_items.push_back(pair.first);
        }
        //        bot.log(dpp::ll_info,
        //               "all_items size: " + std::to_string(all_items.size()));

        // Filter items that start with the user input (case-insensitive)
        std::vector<std::string> matched_items;
        for (const auto& item : all_items) {
          auto it = std::search(
              item.begin(), item.end(), uservalue.begin(), uservalue.end(),
              [&](const unsigned char ch1, const unsigned char ch2) {
                return std::tolower(ch1) == std::tolower(ch2);
              });
          if (it != item.end()) {  // Partial match
            matched_items.push_back(item);
            // bot.log(dpp::ll_info, "match found~! " + item);
          }
        }

        // Add filtered results to autocomplete choices (max 25, per Discord
        // API)
        size_t count = 0;
        for (const auto& match : matched_items) {
          if (count++ >= 25)
            break;
          // bot.log(dpp::ll_info, "adding to response..");
          response.add_autocomplete_choice(
              dpp::command_option_choice(match, match));
        }

        // Send the autocomplete response
        bot.interaction_response_create(event.command.id, event.command.token,
                                        response);
        break;

      } else if (opt.name == "hintloc") {
        std::string uservalue = std::get<std::string>(opt.options[0].value);
        dpp::interaction_response response(dpp::ir_autocomplete_reply);

        std::vector<std::string> all_locations;
        for (const auto& pair : Oot::OOTItemHints::m_itemToLocation) {
          for (const auto& location : pair.second) {
            all_locations.push_back(location);
          }
        }

        std::vector<std::string> matched_locations;
        for (const auto& location : all_locations) {
          auto it = std::search(
              location.begin(), location.end(), uservalue.begin(),
              uservalue.end(),
              [&](const unsigned char ch1, const unsigned char ch2) {
                return std::tolower(ch1) == std::tolower(ch2);
              });
          if (it != location.end()) {  // Partial match
            matched_locations.push_back(location);
          }
        }

        // Add filtered results to autocomplete choices (max 25, per Discord
        // API)
        size_t count = 0;
        for (const auto& match : matched_locations) {
          if (count++ >= 25)
            break;
          response.add_autocomplete_choice(
              dpp::command_option_choice(match, match));
        }

        // Send the autocomplete response
        bot.interaction_response_create(event.command.id, event.command.token,
                                        response);
        break;
      }
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
