#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "tools.h"
#include "packets.h"
#include "worldgen.h"
#include "registries.h"
#include "procedures.h"
#include "serialize.h"
#include "dispatch.h"
#include "dispatch_play.h"

static void handleHandshakePacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00:
			if (cs_handshake(ctx, client_fd)) break;
			break;
		default:
			#ifdef DEV_LOG_UNKNOWN_PACKETS
				printf("Unknown packet: 0x");
				if (packet_id < 16) printf("0");
				printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
			#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handleStatusPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00:
			if (sc_statusResponse(ctx, client_fd)) break;
			break;
		case 0x01:
			writeByte(client_fd, 9);
			writeByte(client_fd, 0x01);
			writeUint64(client_fd, readUint64(ctx, client_fd));
			ctx->recv_count = 0;
			return;
		default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Unknown packet: 0x");
					if (packet_id < 16) printf("0");
					printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
				#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handleLoginPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00: {
			uint8_t uuid[16];
			char name[16];
			if (cs_loginStart(ctx, client_fd, uuid, name)) break;
			if (reservePlayerData(ctx, client_fd, uuid, name)) {
				ctx->recv_count = 0;
				return;
			}
			if (sc_loginSuccess(client_fd, uuid, name)) break;
			break;
		}
		case 0x03:
			printf("Client Acknowledged Login\n\n");
			setClientState(ctx, client_fd, STATE_CONFIGURATION);
			break;
		default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Unknown packet: 0x");
					if (packet_id < 16) printf("0");
					printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
				#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handleConfigurationPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00:
			if (cs_clientInformation(ctx, client_fd)) break;
			if (sc_knownPacks(client_fd)) break;
			if (sc_registries(client_fd)) break;
			#ifdef SEND_BRAND
			if (sc_sendPluginMessage(client_fd, "minecraft:brand", (uint8_t *)ctx->brand, ctx->brand_len)) break;
			#endif
			break;
		case 0x02:
			cs_pluginMessage(ctx, client_fd);
			break;
		case 0x03:
			printf("Client Acknowledged Configuration\n\n");
			completePlayerSpawnSequence(ctx, client_fd);
			break;
		case 0x07:
			printf("Received Client's Known Packs\n");
			printf("  Finishing configuration\n\n");
			sc_finishConfiguration(client_fd);
			completePlayerSpawnSequence(ctx, client_fd);
			break;
		default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Unknown packet: 0x");
					if (packet_id < 16) printf("0");
					printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
				#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

// PLAY sub-handlers moved to separate compilation units

static void handlePlayPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x1D:
		case 0x1E:
		case 0x1F:
		case 0x20:
			handlePlayMovement(ctx, client_fd, length, packet_id);
			return;
		case 0x06:
		case 0x07:
		case 0x08:
			handlePlayChat(ctx, client_fd, length, packet_id);
			return;
		case 0x11:
		case 0x12:
		case 0x19:
		case 0x28:
		case 0x34:
		case 0x3C:
		case 0x3F:
		case 0x40:
			handlePlayInventory(ctx, client_fd, length, packet_id);
			return;
		case 0x0B:
		case 0x1B:
		case 0x29:
		case 0x2A:
		case 0x2B:
			handlePlaySystem(ctx, client_fd, length, packet_id);
			return;
		default:
			#ifdef DEV_LOG_UNKNOWN_PACKETS
				printf("Unknown packet: 0x");
				if (packet_id < 16) printf("0");
				printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
			#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			return;
	}
}

// (Definitions now live in src/dispatch_play_*.c)

// Handle incoming packets; moved here from old c_main.c
void handlePacket (ServerContext *ctx, int client_fd, int length, int packet_id, int state) {
		uint64_t bytes_received_start = total_bytes_received;

		switch (state) {
			case STATE_NONE:
				handleHandshakePacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_STATUS:
				handleStatusPacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_LOGIN:
				handleLoginPacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_CONFIGURATION:
				handleConfigurationPacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_PLAY:
				handlePlayPacket(ctx, client_fd, length, packet_id);
				break;
			default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Packet received for unknown state: 0x%X\n\n", state);
				#endif
				recv_all(client_fd, ctx->recv_buffer, length, false);
				break;
		}

		int processed_length = total_bytes_received - bytes_received_start;
		if (processed_length == length) return;
		if (length > processed_length) {
			recv_all(client_fd, ctx->recv_buffer, length - processed_length, false);
		}
		#ifdef DEV_LOG_LENGTH_DISCREPANCY
		if (processed_length != 0) {
				printf("WARNING: Packet 0x%X parsed incorrectly!\n  Expected: %d, parsed: %d\n\n", packet_id, length, processed_length);
		}
		#endif
}


