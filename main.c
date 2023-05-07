/* SPDX-License-Identifier: MIT */
/* Author: Wojciech Bartczak */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAX_CLIENTS 8

static int done = 0;

void handle(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
        done = 1;
}

int handler_read(int sock)
{
    uint8_t buf[1024];
    int ret = 0;

    printf("reading client!!!\n");

    memset(buf, 0, sizeof(buf));
    ret = recv(sock, buf, sizeof(buf), 0);
    if (ret < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    printf("%s\n", buf);

    ret = send(sock, "OK", sizeof("OK"), 0);
    if (ret < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;
    struct sigaction sa;
    int clients[MAX_CLIENTS];
    int count = 0;

    int acceptor;
    int ret;

    /* Install basic handlers */
    sa.sa_handler = handle;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    ret = sigaction(SIGINT, &sa, NULL);
    if (ret < 0)
        return 1;

    ret = sigaction(SIGTERM, &sa, NULL);
    if (ret < 0)
        return 1;

    acceptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (acceptor < 0)
        return 1;

    memset(&sin, 0 , sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(10555);
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ret = bind(acceptor, (const struct sockaddr*)&sin, sizeof(sin));
    if (ret < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        goto failed;
    }

    memset(clients, 0, sizeof(clients));
    for (;;) {
        fd_set rfds;
        struct timeval tv;
        int lastfd;

        if (done)
            break;

        /* Setup select */
        tv.tv_sec = 0;
        tv.tv_usec = 300 * 1000;

        FD_ZERO(&rfds);
        FD_SET(acceptor, &rfds);

        for (int cnt = 0; cnt < count; cnt++) {
            if (clients[cnt] > 0)
                FD_SET(clients[cnt], &rfds);
        }
        lastfd = count ? clients[count - 1] : acceptor;

        ret = select(lastfd + 1, &rfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR)
                break;

            fprintf(stderr, "Error: %s\n", strerror(errno));
            goto failed;
        }
        if (ret == 0)
            continue;

        if (FD_ISSET(acceptor, &rfds)) {
            uint8_t buf[4];
            struct sockaddr_in ep;
            socklen_t len;

            memset(&ep, 0, sizeof(ep));
            memset(buf, 0, sizeof(buf));
            ret = recvfrom(acceptor, &buf, sizeof(buf), 0,
                           (struct sockaddr *)&ep, &len);
            if (ret < 0) {
                fprintf(stderr, "Error: %s\n", strerror(errno));
                goto failed;
            }

            printf("Connecting from %s:%hu, %s\n",
                   inet_ntoa(ep.sin_addr), ntohs(ep.sin_port), buf);

            ep.sin_family = AF_INET;

            int nfd = dup(acceptor);

            if (nfd < 0) {
                fprintf(stderr, "Error: %s\n", strerror(errno));
                goto failed;
            }

            ret = connect(nfd, (const struct sockaddr*)&ep, sizeof(ep));
            if (ret < 0) {
                fprintf(stderr, "Error: %s\n", strerror(errno));
                goto failed;
            }

            clients[count] = nfd;
            count++;
        }

        for (int cnt = 0; cnt < count; cnt++) {
            if (FD_ISSET(clients[cnt], &rfds)) {
                if (handler_read(clients[cnt]) < 0)
                    goto failed;
            }
        }
    }

    close(acceptor);

    return 0;

failed:
    close(acceptor);

    return 0;
}