#include "U.h"

namespace U {
    dpp::embed createEmbed ( const std::string &description ) {
        return dpp::embed ( )
            .set_color ( dpp::colors::pastel_light_blue )
            .set_title ( "Logos" )
            .set_url ( "https://fizzysylv.xyz/" )
            .set_author ( "Fizzy",
                          "https://fizzysylv.xyz/",
                          "https://genrandom.com/cats" )
            .set_description ( description )
            .set_thumbnail ( "https://genrandom.com/cats" )
            .add_field ( " field title", "Some value here" )
            .add_field ( "Inline field title", "Some value here", true )
            .add_field ( "Inline field title", "Some value here", true )
            .set_image ( "https://genrandom.com/cats" )
            .set_footer ( dpp::embed_footer ( ).set_text ( "meow~" ).set_icon (
                "https://genrandom.com/cats" ) )
            .set_timestamp ( time ( 0 ) );
    }

    void archiveChannel ( const std::string &channelName,
                          const dpp::cache< dpp::message > &cache ) {}
}  // namespace U
