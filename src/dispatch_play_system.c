#include <stdint.h>
#include "globals.h"
#include "tools.h"
#include "packets.h"
#include "procedures.h"
#include "dispatch_play.h"

void handlePlaySystem(ServerContext *ctx, int client_fd, int length, int packet_id) {
  switch (packet_id) {
    case 0x0B: cs_clientStatus(ctx, client_fd); break;
    case 0x1B: recv_all(client_fd, ctx->recv_buffer, length, false); break;
    case 0x29: cs_playerCommand(ctx, client_fd); break;
    case 0x2A: cs_playerInput(ctx, client_fd); break;
    case 0x2B: cs_playerLoaded(ctx, client_fd); break;
    default:
      recv_all(client_fd, ctx->recv_buffer, length, false);
  }
}
