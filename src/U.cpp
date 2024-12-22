#include <Logos/U.h>
#include <dpp/colors.h>

namespace U {

void embed(dpp::cluster& bot,
           uint32_t color,
           dpp::snowflake channel_id,
           const std::string& message) {
  bot.message_create(dpp::message(channel_id, "")
                         .add_embed(dpp::embed()
                                        .set_description(message)
                                        .set_title("Pickle Rick!")
                                        .set_color(color)),
                     [&](const dpp::confirmation_callback_t& callback) {
                       if (callback.is_error()) {
                         bot.log(dpp::ll_error, "Failed to send message: " +
                                                    callback.http_info.body);
                       }
                     });
}

void good_embed(dpp::cluster& bot,
                dpp::snowflake channel_id,
                const std::string& message) {
  embed(bot, 0x7aff7a, channel_id, message);
}

void bad_embed(dpp::cluster& bot,
               dpp::snowflake channel_id,
               const std::string& message) {
  embed(bot, 0xff7a7a, channel_id, message);
}

dpp::embed createEmbed(const U::mType& mType, const std::string& m) {
  switch (mType) {
    case U::mType::GOOD: {
      return dpp::embed()
          .set_color(dpp::colors::iguana_green)
          .set_title("Success!")
          .set_description(m)
          .set_timestamp(time(0));
    }

    case U::mType::BAD: {
      return dpp::embed()
          .set_color(dpp::colors::cranberry)
          .set_title("Uh-Oh!")
          .set_description(m)
          .set_timestamp(time(0));
    }
    default: {
      break;
    }
  }
}

void archiveChannel(const std::string& channelName,
                    const dpp::cache<dpp::message>& cache) {}

bool match(const char* str, const char* mask) {
  char* cp = NULL;
  char* mp = NULL;
  char* string = (char*)str;
  char* wild = (char*)mask;

  while ((*string) && (*wild != '*')) {
    if ((tolower(*wild) != tolower(*string)) && (*wild != '?')) {
      return 0;
    }
    wild++;
    string++;
  }

  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string + 1;
    } else {
      if ((tolower(*wild) == tolower(*string)) || (*wild == '?')) {
        wild++;
        string++;

      } else {
        wild = mp;
        string = cp++;
      }
    }
  }

  while (*wild == '*') {
    wild++;
  }

  return !*wild;
}
}  // namespace U
