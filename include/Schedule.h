#pragma once
#include <dpp/dpp.h>
#include <dpp/snowflake.h>
#include <sqlite3.h>
namespace Schedule {
    static void createEvent ( );
    static bool editEvent ( );
    static bool deleteEvent ( );
    static std::string unixToString ( );
    dpp::snowflake m_userSnowflake;
    sqlite3 *m_db;
    void openDatabase ( );
    void createTable ( );
};  // namespace Schedule
