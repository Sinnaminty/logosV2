cmake_minimum_required(VERSION 3.6)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(BOT_NAME "Logos")

project(${BOT_NAME})
aux_source_directory("src" coresrc)
add_executable(${BOT_NAME} ${coresrc})

string(ASCII 27 Esc)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set_target_properties(${BOT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)
find_package(DPP REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  date_src
  GIT_REPOSITORY https://github.com/HowardHinnant/date.git
  GIT_TAG master
)
FetchContent_MakeAvailable(date_src)

target_include_directories(${BOT_NAME} PUBLIC

    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${DPP_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(${BOT_NAME} PRIVATE
    dl
    fmt
    mpg123
    ${CMAKE_THREAD_LIBS_INIT}
    ${DPP_LIBRARIES}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    date::date
    date::date-tz
)
