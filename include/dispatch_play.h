#ifndef H_DISPATCH_PLAY
#define H_DISPATCH_PLAY

#include "context.h"

void handlePlayMovement(ServerContext *ctx, int client_fd, int length, int packet_id);
void handlePlayChat(ServerContext *ctx, int client_fd, int length, int packet_id);
void handlePlayInventory(ServerContext *ctx, int client_fd, int length, int packet_id);
void handlePlaySystem(ServerContext *ctx, int client_fd, int length, int packet_id);

#endif
