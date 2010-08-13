#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PAD(E) (4 - ((E) % 4)) % 4

const char X_CONNECT[] = {0x6c, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int X_CONNECT_LEN = 12;

struct server {
    int sock;
    char *vendor;
    unsigned int root;
    char *data;
};

void parse_info(struct server *server)
{
    int vendor_len = server->data[24] + (server->data[25] << 8);
    int formats_count = server->data[29];
    
    int vendor_end = 40 + vendor_len;
    server->data[vendor_end] = 0;
    server->vendor = server->data + 40;

    int screens_start = (formats_count * 8) + vendor_end + PAD(vendor_len);
    server->root = *(unsigned int *)(server->data + screens_start);
}

struct server *x_connect(char *sockpath)
{
    struct server *server;
    int size;
    int ret;
    char header[8];
    struct sockaddr_un addr = {AF_UNIX};
    strncpy(addr.sun_path, sockpath, 108);

    server = malloc(sizeof(struct server));

    server->sock = socket(AF_UNIX, SOCK_STREAM, 0);
    ret = connect(server->sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    ret = send(server->sock, X_CONNECT, X_CONNECT_LEN, 0);
    ret = recv(server->sock, header, 8, 0);
    if (1 == header[0]) {
        size = (header[6] + (header[7] << 8)) * 4;
        server->data = malloc(size + 8);
        memcpy(server->data, header, 8);
        ret = recv(server->sock, server->data + 8, size, 0);
        parse_info(server);
        return server;
    }
    return 0;
}

void x_free(struct server *server)
{
    free(server->data);
    free(server);
}

void x_warp_mouse(struct server *server, int x, int y)
{
    char warp[] = {41, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    *(unsigned int *)(warp +8) = server->root;
    *(short *)(warp + 20) = x;
    *(short *)(warp + 22) = y;
    send(server->sock, warp, 24, 0);
}

void usage(char* name)
{
    printf("usage: %s sockpath x y\n", name);
}

int main(int argc, char **argv)
{
    int x, y;
    struct server *server;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    
    x = atoi(argv[2]);
    y = atoi(argv[3]);

    server = x_connect(argv[1]);
    printf("vendor: %s\nroot: %d\n", server->vendor, server->root);
    x_warp_mouse(server, x, y);
    x_free(server);
}
