#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "9000"
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define MAX_BUF 1024

static int server_socket = -1;
static int file_descriptor = -1;

void handle_exit() {
    syslog(LOG_INFO, "Signal received, exiting...");
    if (server_socket != -1) close(server_socket);
    if (file_descriptor != -1) close(file_descriptor);
    unlink(FILE_PATH);
    closelog();
}

void signal_callback(int sig __attribute__((unused))) {
    handle_exit();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    struct addrinfo hints = {0}, *servinfo, *rp;
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    int client_socket;
    char ip_str[INET6_ADDRSTRLEN];
    char temp_buffer[MAX_BUF];
    ssize_t received;
    int run_as_daemon = 0;

    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        run_as_daemon = 1;
    }

    signal(SIGINT, signal_callback);
    signal(SIGTERM, signal_callback);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0) {
        syslog(LOG_ERR, "getaddrinfo failed");
        return EXIT_FAILURE;
    }

    for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
        server_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server_socket == -1) continue;

        if (bind(server_socket, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(server_socket);
    }

    freeaddrinfo(servinfo);

    if (rp == NULL) {
        syslog(LOG_ERR, "Failed to bind to any address");
        return EXIT_FAILURE;
    }

    if (run_as_daemon) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "Failed to fork");
            return EXIT_FAILURE;
        }
        if (pid > 0) return EXIT_SUCCESS;

        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    if (listen(server_socket, 5) == -1) {
        syslog(LOG_ERR, "Listen error");
        return EXIT_FAILURE;
    }

    while (1) {
        addr_len = sizeof client_addr;
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == -1) {
            if (errno == EINTR) break;
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        void *addr_ptr = (client_addr.ss_family == AF_INET) ?
            (void *)&(((struct sockaddr_in *)&client_addr)->sin_addr) :
            (void *)&(((struct sockaddr_in6 *)&client_addr)->sin6_addr);

        inet_ntop(client_addr.ss_family, addr_ptr, ip_str, sizeof ip_str);
        syslog(LOG_INFO, "Connection established with %s", ip_str);

        file_descriptor = open(FILE_PATH, O_RDWR | O_CREAT | O_APPEND, 0644);
        if (file_descriptor == -1) {
            syslog(LOG_ERR, "Failed to open or create file");
            close(client_socket);
            continue;
        }

        while ((received = recv(client_socket, temp_buffer, MAX_BUF, 0)) > 0) {
            for (ssize_t i = 0; i < received; ++i) {
                write(file_descriptor, &temp_buffer[i], 1);
                if (temp_buffer[i] == '\n') {
                    lseek(file_descriptor, 0, SEEK_SET);
                    char read_buf[MAX_BUF];
                    ssize_t bytes_read;
                    while ((bytes_read = read(file_descriptor, read_buf, MAX_BUF)) > 0) {
                        send(client_socket, read_buf, bytes_read, 0);
                    }
                    lseek(file_descriptor, 0, SEEK_END);
                }
            }
        }

        close(file_descriptor);
        file_descriptor = -1;
        close(client_socket);
        syslog(LOG_INFO, "Connection with %s closed", ip_str);
    }

    handle_exit();
    return EXIT_SUCCESS;
}
