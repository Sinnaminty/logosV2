#include <Logos/Logos.h>
#include <dpp/colors.h>
#include <mpg123.h>
#include <cstddef>

using json = nlohmann::json;

namespace Logos {

dpp::embed createEmbed(const mType& mType, const std::string& m) {
  switch (mType) {
    case mType::GOOD: {
      return dpp::embed()
          .set_color(dpp::colors::iguana_green)
          .set_title("Success!")
          .set_description(m)
          .set_timestamp(time(0));
    }

    case mType::BAD: {
      return dpp::embed()
          .set_color(dpp::colors::cranberry)
          .set_title("Uh-Oh!")
          .set_description(m)
          .set_timestamp(time(0));
    }
    case mType::EVENT: {
      return dpp::embed()
          .set_color(dpp::colors::bright_gold)
          .set_title("Event!")
          .set_description(m)
          .set_timestamp(time(0));
    }
    default: {
      return dpp::embed();
      break;
    }
  }
}

std::vector<uint8_t> encodeAudio(const std::string& file) {
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

}  // namespace Logos
