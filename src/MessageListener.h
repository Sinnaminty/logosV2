#pragma once
#include <dpp/dispatcher.h>

class MessageListener {
public:
  static void on_message_create(const dpp::message_create_t &event);

  static void on_message_delete(const dpp::message_delete_t &event);

  static void on_message_delete_bulk(const dpp::message_delete_bulk_t &event);

  static void
  on_message_poll_vote_add(const dpp::message_poll_vote_add_t &event);

  static void
  on_message_poll_vote_remove(const dpp::message_poll_vote_remove_t &event);

  static void on_message_reaction_add(const dpp::message_reaction_add_t &event);

  static void
  on_message_reaction_remove(const dpp::message_reaction_remove_t &event);

  static void on_message_reaction_remove_all(
      const dpp::message_reaction_remove_all_t &event);

  static void on_message_reaction_remove_emoji(
      const dpp::message_reaction_remove_emoji_t &event);

  static void on_message_update(const dpp::message_update_t &event);
};
