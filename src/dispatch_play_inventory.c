#include <stdint.h>
#include "globals.h"
#include "tools.h"
#include "packets.h"
#include "procedures.h"
#include "dispatch_play.h"

void handlePlayInventory(ServerContext *ctx, int client_fd, int length, int packet_id) {
  switch (packet_id) {
    case 0x11: cs_clickContainer(ctx, client_fd); break;
    case 0x12: cs_closeContainer(ctx, client_fd); break;
    case 0x19: cs_interact(ctx, client_fd); break;
    case 0x28: cs_playerAction(ctx, client_fd); break;
    case 0x34: cs_setHeldItem(ctx, client_fd); break;
    case 0x3C: cs_swingArm(ctx, client_fd); break;
    case 0x3F: cs_useItemOn(ctx, client_fd); break;
    case 0x40: cs_useItem(ctx, client_fd); break;
    default:
      recv_all(client_fd, ctx->recv_buffer, length, false);
  }
}
