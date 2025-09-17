#ifndef H_DISPATCH
#define H_DISPATCH

#include "context.h"

void handlePacket (ServerContext *ctx, int client_fd, int length, int packet_id, int state);

#endif
