#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

struct connection {
    int sock;
};

static void
net_connect(struct connection *conn, struct url url) {
    int ret;

    struct str host = str_until_char(url.rest, '/');
    struct str path = str_after(url.rest, host.len);
    if (path.len == 0) {
        path = STR("/");
    }

    int sock;
    {
        char host_cstr[0x100];
        snprintf(host_cstr, sizeof host_cstr, "%.*s", (int)host.len, host.ptr);
        struct addrinfo hint = {
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *addrs;
        ret = getaddrinfo(host_cstr, "80", &hint, &addrs);
        if (ret != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
            return;
        }
        struct addrinfo *a;
        for (a = addrs; a; a = a->ai_next) {
            sock = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            if (sock == -1) {
                continue;
            }
            ret = connect(sock, a->ai_addr, a->ai_addrlen);
            if (ret != -1) {
                break;
            }
            close(sock);
        }
        if (a == NULL) {
            fprintf(stderr, "Could not get addrinfo\n");
            return;
        }
        freeaddrinfo(addrs);
    }

    conn->sock = sock;
}

static void
send_http_request(struct connection conn, struct url url) {
    struct str host = str_until_char(url.rest, '/');
    struct str path = str_after(url.rest, host.len);
    if (path.len == 0) {
        path = STR("/");
    }

    char send_buf[0x100];
    snprintf(send_buf, sizeof send_buf,
            "GET %.*s HTTP/1.1\r\n"
            "Host: %.*s\r\n"
            "\r\n",
            (int)path.len, path.ptr,
            (int)host.len, host.ptr);
    printf("%s", send_buf);
    send(conn.sock, send_buf, ARR_LEN(send_buf) - 1, 0);
}
