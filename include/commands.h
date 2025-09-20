#ifndef COMMANDS_H
#define COMMANDS_H

#include "globals.h"
#include "packets.h"
#include "procedures.h"

char *getArgument ();
char *getRemainingArguments ();
void handleCommand (PlayerData *sender, int message_len);
#ifdef WHITELIST
  void handleWhitelistCommand (PlayerData *sender);
#endif
void handleMessageCommand (PlayerData *sender);
void handleHelpCommand (PlayerData *sender);

#endif
