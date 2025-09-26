#ifndef COMMANDS_H
#define COMMANDS_H

#include "globals.h"
#include "packets.h"
#include "procedures.h"

char *getNextArgument ();
char *getRemainingArguments ();
void handleCommand (PlayerData *sender, int message_len);
#if MAX_WHITELISTED_PLAYERS > 0
  void handleWhitelistCommand (PlayerData *sender);
#endif
void handleMessageCommand (PlayerData *sender);
void handleHelpCommand (PlayerData *sender);

#endif
