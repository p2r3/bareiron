#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "commands.h"

void handleCommand (PlayerData *player, int message_len) {
  char* command = strtok((char *)recv_buffer, " ");
  if (!strcmp(command, "!help")) {
    handleHelpCommand(player);
  } else if (!strcmp(command, "!msg")) {
    handleMessageCommand(player);
  #if MAX_WHITELISTED_PLAYERS > 0
  } else if (!strcmp(command, "!whitelist")) {
    handleWhitelistCommand(player);
  #endif
  } else {
    sc_systemChat(player->client_fd, "§7Unknown command. Type !help for help.", 40);
  }
}

// Pulls a single space-delimited argument from the chat buffer
char *getNextArgument () {
  return strtok(NULL, " ");
}

// Pulls the remaining text from the chat buffer
char *getRemainingArguments () {
  return strtok(NULL, "");
}

#if MAX_WHITELISTED_PLAYERS > 0
void handleWhitelistCommand (PlayerData *player) {
  char *first_arg = getNextArgument();

  if (first_arg == NULL) {
    goto usage;
  }

  if (!strcmp(first_arg, "on")) {
    sc_systemChat(player->client_fd, "§aWhitelist has been enabled", 29);
    enforce_whitelist = 1;
    return;
  } else if (!strcmp(first_arg, "off")) {
    sc_systemChat(player->client_fd, "§cWhitelist has been disabled", 30);
    enforce_whitelist = 0;
    return;
  } else if (!strcmp(first_arg, "add")) {
    char* player_arg = getNextArgument();
    if (player_arg == NULL) {
      goto usage;
    }
    int player_len = strlen(player_arg);
    if (player_len < 3 || player_len > 16) {
        sc_systemChat(player->client_fd, "§cError: input username is invalid", 35);
        return;
    }
    int result = addPlayerToWhitelist(player_arg);
    if (result == 0) {
      sc_systemChat(player->client_fd, "§7Successfully added player to the whitelist", 45);
    } else if (result == 1) {
      sc_systemChat(player->client_fd, "§cError: Player is already on the whitelist", 44);
    } else {
      sc_systemChat(player->client_fd, "§cError: The whitelist is full, remove other players first then try again", 74);
    }
    return;
  } else if (!strcmp(first_arg, "remove")) {
    char* player_arg = getNextArgument();
    if (player_arg == NULL) {
      goto usage;
    }
    int player_len = strlen(player_arg);
    if (player_len < 3 || player_len > 16) {
        sc_systemChat(player->client_fd, "§cError: input username is invalid", 35);
        return;
    }
    if (removePlayerFromWhitelist(player_arg) == 0) {
      sc_systemChat(player->client_fd, "§7Successfully removed player from the whitelist", 49);
    } else {
      sc_systemChat(player->client_fd, "§cError: Player is not on the whitelist", 40);
    }
    return;
  } else if (!strcmp(first_arg, "list")) {
    snprintf((char *)recv_buffer, sizeof(recv_buffer), "§7The currently whitelisted players are:");
    recv_buffer[41] = ' ';
    int length = 42;
    for (int i = 0; i < MAX_WHITELISTED_PLAYERS; i ++) {
      if (whitelisted_players[i][0] == '\0') continue;

      snprintf((char *)recv_buffer + length, sizeof(recv_buffer) - length, "%s, ", whitelisted_players[i]);
      length += strlen(whitelisted_players[i]) + 2;
    }
    if (length == 42){
        sc_systemChat(player->client_fd, "§7There are currently no whitelisted players", 45);
        return;
    }
    // Subtract 2 from the length to remove the trailing comma and space
    sc_systemChat(player->client_fd, (char *)recv_buffer, length - 2);
    return;
  }
usage:
  sc_systemChat(player->client_fd, "§7Usage: !whitelist <on|off|add|remove> [username]", 51);
}
#endif

void handleMessageCommand (PlayerData* player) {
  char* target_name = getNextArgument();

  // Send usage guide if arguments are missing
  if (target_name == NULL) goto usage;

  // Query the target player
  PlayerData *target = getPlayerByName(target_name, recv_buffer);
  if (target == NULL) {
    sc_systemChat(player->client_fd, "Player not found", 16);
    return;
  }

  char *message = getRemainingArguments();

  // Don't send empty messages
  if (message == NULL) goto usage;

  // Format output as a vanilla whisper
  int name_len = strlen(player->name);
  int text_len = strlen(message);
  memmove(recv_buffer + name_len + 24, message, text_len);
  snprintf((char *)recv_buffer, sizeof(recv_buffer), "§7§o%s whispers to you:", player->name);

  // snprintf always null terminates strings, so we get rid of that here
  recv_buffer[name_len + 23] = ' ';

  // Send message to target player
  sc_systemChat(target->client_fd, (char *)recv_buffer, (uint16_t)(name_len + 24 + text_len));

  // Format output for sending player
  int target_name_len = strlen(target->name);
  memmove(recv_buffer + target_name_len + 23, recv_buffer + name_len + 24, text_len);
  snprintf((char *)recv_buffer, sizeof(recv_buffer), "§7§oYou whisper to %s:", target->name);
  recv_buffer[target_name_len + 22] = ' ';

  // Report back to sending player
  sc_systemChat(player->client_fd, (char *)recv_buffer, (uint16_t)(target_name_len + 23 + text_len));
  return;

usage:
  sc_systemChat(player->client_fd, "§7Usage: !msg <player> <message>", 33);
}

void handleHelpCommand (PlayerData *player) {
  // Send command guide
  const char help_msg[] = "§7Commands:\n"
  "  !msg <player> <message> - Send a private message\n"
  #if MAX_WHITELISTED_PLAYERS > 0
  "  !whitelist <on|off|add|remove> [username]\n"
  #endif
  "  !help - Show this help message";
  sc_systemChat(player->client_fd, (char *)help_msg, (uint16_t)sizeof(help_msg) - 1);
}
