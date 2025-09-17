#include <stdint.h>
#include "globals.h"
#include "tools.h"
#include "packets.h"
#include "procedures.h"
#include "dispatch_play.h"

void handlePlayChat(ServerContext *ctx, int client_fd, int length, int packet_id) {
  switch (packet_id) {
    case 0x06:
    case 0x07:
      cs_chatCommand(ctx, client_fd);
      break;
    case 0x08:
      cs_chat(ctx, client_fd);
      break;
    default:
      recv_all(client_fd, ctx->recv_buffer, length, false);
  }
}
