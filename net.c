#include "net.h"
#include "jbod.h"
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
    int num_read = 0;
    while (num_read < len) {
        int num = read(fd, &buf[num_read], len - num_read);
        if (num < 0) return false;
        if (num >= 0) num_read = num;
        len -= num_read;
    }
    return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
    int num_writ = 0;
    while (num_writ < len) {
        int num = write(fd, &buf[num_writ], len - num_writ);
        if (num < 0) return false;
        if (num >= 0) {
            num_writ = num;
            len -= num_writ;
        }
    }
    return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
    // decode values received from the network
    // dont know about sd op block and all packet stuff
    // modifying pointers
    /// nread to get packet header and then decode packet header

    // call nread on header length(8)
    // modify pointers using memcpy(arguments. packet ...+ arguments, size)
    // ntoh conversions
    // if there is a block(payload) nread( ,256, )

    uint16_t len = 0;
    uint8_t header[8];

    nread(fd, 8, header);
    memcpy(&len, header, 2);
    len = ntohs(len);
    memcpy(op, header + 2, 4);
    *op = ntohl(*op);
    memcpy(ret, header + 6, 2);
    *ret = ntohs(*ret);
    if (len == 8) return true;
    return nread(fd, 256, block);
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
    // create packet header+payload(optional) header is len, op,
    // ret, block copy contents into packet but first convert with
    //  hton(op)...
    // fill packet with memcpy(packet, headerlength, numofbytes)
    //   memcpy(packet + headerlength, )
    //   call nwrite(sd, len, buf)

    // create packet
    uint16_t len = 8;
    uint32_t packet[264];
    // shift op left 26 bits
    int cmd = op >> 26;
    // packet is either size header_len or header_len + blocksize. Check with
    // opcode (writeblock)
    if (cmd == JBOD_WRITE_BLOCK) {
        len = 264;
        uint16_t templen = htons(len);
        uint32_t tempop = htonl(op);
        memcpy(packet, &templen, 2);
        memcpy(packet + 2, tempop, 4);
        memcpy(packet + 8, block, 256);
        return nwrite(sd, len, block);
    } else {
        len = 264;
        uint16_t templen = htons(len);
        uint32_t tempop = htonl(op);
        memcpy(packet, &templen, 2);
        memcpy(packet + 2, tempop, 4);
        return nwrite(sd, len, block);
    }
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
    struct sockaddr_in client_address;

    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(JBOD_PORT);

    if (inet_aton(ip, &(client_address.sin_addr)) == 0) {
        return false;
    }

    if ((cli_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return false;
    }

    if (connect(cli_sd, (struct sockaddr *)&client_address,
                sizeof(client_address)) == -1) {
        return false;
    }

    return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
    close(cli_sd);
    cli_sd = -1;
    return;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
    // call send and receive packet
    bool t, s = false;
    uint16_t *ret;
    uint32_t *newop;

    t = send_packet(cli_sd, op, block);
    if (t == true) {
        s = recv_packet(cli_sd, newop, ret, block);
        if (s == false) {
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}
