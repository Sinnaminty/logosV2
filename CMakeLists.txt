cmake_minimum_required(VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(BOT_NAME "Logos")

project(${BOT_NAME}
  VERSION 0.0.1
  DESCRIPTION "Discord Bot built using the DPP API Lib. Created by Fizzy."
)

file(GLOB_RECURSE src "src/*.cpp")
add_executable(${BOT_NAME} ${src})

string(ASCII 27 Esc)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_target_properties(${BOT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(DPP)
find_package(OpenSSL REQUIRED)

target_include_directories(${BOT_NAME} PUBLIC
  ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(${BOT_NAME}
  dl
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENSSL_CRYPTO_LIBRARY}
  ${OPENSSL_SSL_LIBRARY}
)

if(DPP_FOUND)
  target_link_libraries(${BOT_NAME} ${DPP_LIBRARIES})
  target_include_directories(${BOT_NAME} PUBLIC ${DPP_INCLUDE_DIR})
else()
  message(WARNING "Could not find DPP install. Building from source instead.")
  option(DPP_BUILD_TEST "" OFF)
  include(FetchContent)
  FetchContent_Declare(
    libdpp
    GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
    GIT_TAG master
  )

  FetchContent_GetProperties(libdpp)
  if(NOT libdpp_POPULATED)
    FetchContent_Populate(libdpp)

    target_include_directories(${BOT_NAME} PUBLIC
      ${libdpp_SOURCE_DIR}/include
    )
    add_subdirectory(
      ${libdpp_SOURCE_DIR}
      ${libdpp_BINARY_DIR}
      EXCLUDE_FROM_ALL
    )
  endif()

  target_link_libraries(${BOT_NAME} dpp)
endif()
