/* Native load generator and event-driven FastCGI fixture for source benchmarks.
 * Build: cc -O2 -Wall -Wextra -std=c99 -o source_reuse_bench source_reuse_bench.c
 * The backend creates no process/thread per accepted connection. */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BODY_SIZE 128
#define MAX_PEERS 64

static void die(const char *message) { perror(message); exit(1); }
static uint64_t now_ns(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) die("clock_gettime");
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
static int transfer(int fd, void *buffer, size_t size, int writing) {
    char *p = buffer;
    while (size) {
        ssize_t n = writing ? write(fd, p, size) : read(fd, p, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n; size -= (size_t)n;
    }
    return 0;
}
static void nodelay(int fd) {
    int one = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one))) die("TCP_NODELAY");
}
static void nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK)) die("nonblocking");
}
static double cpu_seconds(void) {
    struct rusage r;
    if (getrusage(RUSAGE_SELF, &r)) die("getrusage");
    return r.ru_utime.tv_sec + r.ru_stime.tv_sec + (r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1e6;
}

typedef struct {
    unsigned char input[8192], params[8192], output[512];
    size_t used, param_size, output_size, sent;
    uint64_t connection;
    unsigned int id, keep;
} peer_t;

static size_t fcgi_record(unsigned char *out, unsigned int kind, unsigned int id,
                          const void *data, size_t size) {
    unsigned char header[8] = {1, kind, id >> 8, id & 255, size >> 8, size & 255, 0, 0};
    memcpy(out, header, 8);
    if (size) memcpy(out + 8, data, size);
    return size + 8;
}
static int param_length(const unsigned char *data, size_t size, size_t *offset, size_t *length) {
    if (*offset >= size) return -1;
    *length = data[(*offset)++];
    if (*length & 128) {
        if (size - *offset < 3) return -1;
        *length = ((*length & 127) << 24) | ((size_t)data[*offset] << 16) |
                  ((size_t)data[*offset+1] << 8) | data[*offset+2];
        *offset += 3;
    }
    return 0;
}
static int respond(peer_t *p) {
    size_t offset = 0;
    char body[BODY_SIZE];
    int found = 0;
    memset(body, 'A', sizeof(body));
    while (offset < p->param_size) {
        size_t key, value;
        if (param_length(p->params, p->param_size, &offset, &key) ||
            param_length(p->params, p->param_size, &offset, &value) ||
            key > p->param_size-offset || value > p->param_size-offset-key) return -1;
        if (key == 12 && !memcmp(p->params+offset, "QUERY_STRING", key)) {
            if (value != 22 || memcmp(p->params+offset+key, "token=", 6)) return -1;
            memcpy(body, p->params+offset+key+6, 16);
            found = 1;
        }
        offset += key + value;
    }
    if (!found) return -1;
    snprintf(body+16, 17, "%016llx", (unsigned long long)p->connection);
    body[32] = 'A';
    const char header[] = "Content-Type: application/octet-stream\r\nContent-Length: 128\r\n\r\n";
    unsigned char content[sizeof(header)-1+BODY_SIZE], end[8] = {0};
    memcpy(content, header, sizeof(header)-1);
    memcpy(content+sizeof(header)-1, body, sizeof(body));
    p->output_size = fcgi_record(p->output, 6, p->id, content, sizeof(content));
    p->output_size += fcgi_record(p->output+p->output_size, 6, p->id, NULL, 0);
    p->output_size += fcgi_record(p->output+p->output_size, 3, p->id, end, sizeof(end));
    p->sent = 0;
    return 0;
}
static int parse(peer_t *p) {
    size_t offset = 0;
    while (p->used-offset >= 8) {
        const unsigned char *head = p->input+offset;
        size_t size = ((size_t)head[4] << 8) | head[5], total = 8 + size + head[6];
        unsigned int id = ((unsigned int)head[2] << 8) | head[3];
        if (total > p->used-offset) break;
        if (head[0] != 1 || !id) return -1;
        if (head[1] == 1) {
            if (size != 8 || head[8] != 0 || head[9] != 1) return -1;
            p->id = id; p->keep = head[10] & 1; p->param_size = 0;
        } else if (id != p->id) return -1;
        else if (head[1] == 4) {
            if (size > sizeof(p->params)-p->param_size) return -1;
            memcpy(p->params+p->param_size, head+8, size); p->param_size += size;
        } else if (head[1] == 5 && size == 0) {
            if (respond(p)) return -1;
        } else return -1;
        offset += total;
    }
    memmove(p->input, p->input+offset, p->used-offset); p->used -= offset;
    return p->used == sizeof(p->input) ? -1 : 0;
}
static int backend(const char *transport, const char *address) {
    int unix_socket = !strcmp(transport, "unix");
    int listener = socket(unix_socket ? AF_UNIX : AF_INET, SOCK_STREAM, 0);
    if (listener < 0) die("socket");
    if (unix_socket) {
        struct sockaddr_un addr = {0}; addr.sun_family = AF_UNIX;
        if (strlen(address) >= sizeof(addr.sun_path)) { errno = ENAMETOOLONG; die("Unix path"); }
        strcpy(addr.sun_path, address);
        if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) die("bind Unix");
    } else {
        struct sockaddr_in addr = {0}; addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) die("bind TCP");
    }
    if (listen(listener, 128)) die("listen");
    nonblocking(listener);
    if (unix_socket) printf("{\"address\":\"%s\"}\n", address);
    else {
        struct sockaddr_in addr; socklen_t size = sizeof(addr);
        if (getsockname(listener, (struct sockaddr *)&addr, &size)) die("getsockname");
        printf("{\"address\":\"127.0.0.1:%u\"}\n", ntohs(addr.sin_port));
    }
    fflush(stdout);
    struct pollfd fds[MAX_PEERS+2]; peer_t *peers[MAX_PEERS+2] = {0};
    for (int i = 0; i < MAX_PEERS+2; i++) { fds[i].fd = -1; fds[i].events = POLLIN; }
    fds[0].fd = listener; fds[1].fd = STDIN_FILENO;
    unsigned long accepted = 0, requests = 0, errors = 0, live = 0, peak = 0;
    char command[64]; size_t command_size = 0;
    for (;;) {
        if (poll(fds, MAX_PEERS+2, -1) < 0) { if (errno == EINTR) continue; die("poll"); }
        if (fds[1].revents) {
            char c; ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n <= 0 || c == 'q') break;
            if (c == '\n') {
                printf("{\"accepted\":%lu,\"requests\":%lu,\"errors\":%lu,\"live\":%lu,\"peak\":%lu,\"cpu_seconds\":%.9f}\n",
                       accepted, requests, errors, live, peak, cpu_seconds());
                fflush(stdout); command_size = 0;
            } else if (command_size < sizeof(command)) command[command_size++] = c;
        }
        if (fds[0].revents & POLLIN) {
            for (;;) {
                int fd = accept(listener, NULL, NULL);
                if (fd < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK) break; if (errno == EINTR) continue; die("accept"); }
                if (!unix_socket) nodelay(fd);
                nonblocking(fd);
                int slot;
                for (slot = 2; slot < MAX_PEERS+2 && fds[slot].fd >= 0; slot++);
                if (slot == MAX_PEERS+2) { close(fd); errors++; continue; }
                peers[slot] = calloc(1, sizeof(peer_t)); if (!peers[slot]) die("calloc");
                peers[slot]->connection = accepted + 1;
                fds[slot].fd = fd; fds[slot].events = POLLIN; accepted++; live++;
                if (live > peak) peak = live;
            }
        }
        for (int i = 2; i < MAX_PEERS+2; i++) {
            if (fds[i].fd < 0 || !fds[i].revents) continue;
            peer_t *p = peers[i]; int closed = 0;
            if (fds[i].revents & (POLLERR | POLLNVAL)) { errors++; closed = 1; }
            if (!closed && fds[i].revents & (POLLIN | POLLHUP)) {
                ssize_t n = recv(fds[i].fd, p->input+p->used, sizeof(p->input)-p->used, 0);
                if (n == 0) closed = 1;
                else if (n > 0) {
                    p->used += (size_t)n;
                    if (parse(p)) { errors++; closed = 1; }
                    else if (p->output_size) fds[i].events = POLLOUT;
                } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) { errors++; closed = 1; }
            }
            /* Try the write immediately as well as on POLLOUT, for both modes. */
            if (!closed && p->output_size) {
                ssize_t n = send(fds[i].fd, p->output+p->sent, p->output_size-p->sent, 0);
                if (n > 0) p->sent += (size_t)n;
                else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) { errors++; closed = 1; }
                if (p->sent == p->output_size) {
                    requests++; p->output_size = p->sent = 0; fds[i].events = POLLIN;
                    if (!p->keep) closed = 1;
                }
            }
            if (closed) { close(fds[i].fd); fds[i].fd = -1; free(p); peers[i] = NULL; live--; }
        }
    }
    return 0;
}

typedef struct { uint64_t finished_ns; double cpu; unsigned int completed, connections, errors; } result_t;
static int frontend_connect(unsigned int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); if (fd < 0) die("client socket");
    struct timeval timeout = {10, 0};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) die("socket timeout");
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); nodelay(fd);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) { close(fd); return -1; }
    return fd;
}
static void client_worker(unsigned int port, unsigned int count, unsigned int worker, int gate, int out) {
    char start; if (transfer(gate, &start, 1, 0)) _exit(1); close(gate);
    uint64_t *latencies = calloc(count, sizeof(*latencies)), *backends = calloc(count, sizeof(*backends));
    if (!latencies || !backends) die("latencies/connections");
    result_t result = {0}; int fd = frontend_connect(port);
    if (fd >= 0) result.connections++;
    for (unsigned int i = 0; i < count && fd >= 0; i++) {
        char token[17], request[256], response[16385]; size_t used = 0, expected = 0;
        snprintf(token, sizeof(token), "%08x%08x", (unsigned int)getpid() ^ worker, i);
        int length = snprintf(request, sizeof(request), "GET /bench.php?token=%s HTTP/1.1\r\nHost: bench\r\nConnection: keep-alive\r\n\r\n", token);
        uint64_t began_request = now_ns();
        if (transfer(fd, request, (size_t)length, 1)) { result.errors++; break; }
        char *body = NULL;
        for (;;) {
            ssize_t n = recv(fd, response+used, sizeof(response)-1-used, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) break;
            used += (size_t)n; response[used] = 0;
            if (!body && (body = strstr(response, "\r\n\r\n"))) {
                char *content_length = strcasestr(response, "\r\nContent-Length:");
                if (!content_length || content_length >= body ||
                    strtoul(content_length+17, NULL, 10) != BODY_SIZE ||
                    memcmp(response, "HTTP/1.1 200 ", 13)) break;
                body += 4; expected = (size_t)(body-response) + BODY_SIZE;
            }
            if ((expected && used >= expected) || used == sizeof(response)-1) break;
        }
        if (!expected || used != expected || memcmp(body, token, 16)) { result.errors++; break; }
        uint64_t connection = 0;
        for (int j = 16; j < 32; j++) {
            unsigned int digit;
            if (body[j] >= '0' && body[j] <= '9') digit = body[j]-'0';
            else if (body[j] >= 'a' && body[j] <= 'f') digit = body[j]-'a'+10;
            else { result.errors++; break; }
            connection = (connection << 4) | digit;
        }
        for (int j = 32; j < BODY_SIZE; j++) if (body[j] != 'A') { result.errors++; break; }
        if (!connection) result.errors++;
        if (result.errors) break;
        backends[result.completed] = connection;
        latencies[result.completed++] = now_ns()-began_request;
        if (result.completed % 100000 == 0)
            fprintf(stderr, "worker %u: %u/%u validated requests\n", worker, result.completed, count);
    }
    if (fd < 0) result.errors++;
    else close(fd);
    result.finished_ns = now_ns(); result.cpu = cpu_seconds();
    if (transfer(out, &result, sizeof(result), 1) || transfer(out, latencies, count*sizeof(*latencies), 1) ||
        transfer(out, backends, count*sizeof(*backends), 1)) _exit(1);
    free(latencies); free(backends); close(out); _exit(0);
}
static int compare_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}
static int load(unsigned int port, unsigned int total, unsigned int concurrency) {
    if (!total || !concurrency || concurrency > MAX_PEERS || total % concurrency) { fprintf(stderr, "Invalid load parameters\n"); return 1; }
    int gate[2], pipes[MAX_PEERS][2]; pid_t children[MAX_PEERS];
    if (pipe(gate)) die("gate");
    uint64_t *latencies = calloc(total, sizeof(*latencies)), *backends = calloc(total, sizeof(*backends));
    if (!latencies || !backends) die("latencies/connections");
    for (unsigned int i = 0; i < concurrency; i++) {
        if (pipe(pipes[i])) die("pipe");
        children[i] = fork(); if (children[i] < 0) die("fork");
        if (!children[i]) {
            close(gate[1]); close(pipes[i][0]);
            for (unsigned int j = 0; j < i; j++) close(pipes[j][0]);
            client_worker(port, total/concurrency, i, gate[0], pipes[i][1]);
        }
        close(pipes[i][1]);
    }
    close(gate[0]); char starts[MAX_PEERS] = {0};
    uint64_t began = now_ns();
    if (transfer(gate[1], starts, concurrency, 1)) die("release gate");
    close(gate[1]);
    result_t combined = {0};
    for (unsigned int i = 0; i < concurrency; i++) {
        result_t r;
        if (transfer(pipes[i][0], &r, sizeof(r), 0) ||
            transfer(pipes[i][0], latencies+i*(total/concurrency), (total/concurrency)*sizeof(*latencies), 0) ||
            transfer(pipes[i][0], backends+i*(total/concurrency), (total/concurrency)*sizeof(*backends), 0)) die("worker result");
        close(pipes[i][0]); int status;
        if (waitpid(children[i], &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status)) die("worker failed");
        if (r.finished_ns > combined.finished_ns) combined.finished_ns = r.finished_ns;
        combined.cpu += r.cpu; combined.completed += r.completed; combined.connections += r.connections; combined.errors += r.errors;
    }
    if (combined.completed != total || combined.errors) { fprintf(stderr, "Completed %u/%u, errors %u\n", combined.completed, total, combined.errors); return 1; }
    qsort(latencies, total, sizeof(*latencies), compare_u64);
    qsort(backends, total, sizeof(*backends), compare_u64);
    unsigned int unique = 1;
    for (unsigned int i = 1; i < total; i++) if (backends[i] != backends[i-1]) unique++;
    long double sum = 0; for (unsigned int i = 0; i < total; i++) sum += latencies[i];
    double seconds = (combined.finished_ns-began) / 1e9;
    printf("{\"requests\":%u,\"concurrency\":%u,\"errors\":%u,\"frontend_connections\":%u,\"backend_connections_used\":%u,\"seconds\":%.9f,\"requests_per_second\":%.3f,\"client_cpu_seconds\":%.9f,\"latency_mean_ms\":%.6Lf,\"latency_p50_ms\":%.6f,\"latency_p95_ms\":%.6f,\"latency_p99_ms\":%.6f}\n",
           total, concurrency, combined.errors, combined.connections, unique, seconds, total/seconds, combined.cpu,
           sum/total/1e6L, latencies[(total-1)*50/100]/1e6, latencies[(total-1)*95/100]/1e6, latencies[(total-1)*99/100]/1e6);
    free(latencies); free(backends); return 0;
}
int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    if (argc == 4 && !strcmp(argv[1], "backend")) return backend(argv[2], argv[3]);
    if (argc == 5 && !strcmp(argv[1], "load")) return load(strtoul(argv[2], NULL, 10), strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
    fprintf(stderr, "Usage: %s backend tcp|unix PATH | load PORT REQUESTS CONCURRENCY\n", argv[0]); return 1;
}
