pub const c = @cImport({
    @cDefine("_NO_CRT_STDIO_INLINE", "1");

    @cInclude("stdio.h");
    @cInclude("stdlib.h");
    @cInclude("stdint.h");

    // OS networking headers for fd_set/select and socket functions
    if (@import("builtin").target.os.tag == .windows) {
        @cInclude("winsock2.h");
        @cInclude("ws2tcpip.h");
    } else {
        @cInclude("sys/socket.h");
        @cInclude("arpa/inet.h");
        @cInclude("fcntl.h");
        @cInclude("poll.h");
    }

    @cInclude("globals.h");
    @cInclude("context.h");
    @cInclude("tools.h");
    @cInclude("varnum.h");
    @cInclude("packets.h");
    @cInclude("procedures.h");
    @cInclude("serialize.h");
    @cInclude("dispatch.h");
});