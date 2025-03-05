#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/select.h>

#define SOCKET_PATH "/tmp/trusted_daemon.sock"
#define BUFFER_SIZE 1024

// Function to verify user credentials
bool verify_credentials(const char *username, const char *password) {
    sqlite3 *db;
    int rc = sqlite3_open("users.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return false;
    }

    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM users WHERE username='%s' AND password='%s';", username, password);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    bool result = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = true; // Credentials are valid
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

// Function to check if the user has permission to access the resource
bool check_permission(const char *username, const char *resource) {
    sqlite3 *db;
    int rc = sqlite3_open("users.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return false;
    }

    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM permissions WHERE username='%s' AND resource='%s';", username, resource);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    bool result = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = true; // User has permission
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

// Function to send a message to the client
void send_message(int client_fd, const char *message) {
    write(client_fd, message, strlen(message));
}

// Function to send a file descriptor to the client
void send_fd(int socket, int fd_to_send) {
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(fd_to_send))];
    memset(buf, 0, sizeof(buf));

    struct iovec io = { .iov_base = "FD", .iov_len = 2 };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd_to_send));

    *((int *) CMSG_DATA(cmsg)) = fd_to_send;

    if (sendmsg(socket, &msg, 0) < 0) {
        perror("Failed to send file descriptor");
    }
}

// Function to handle client requests
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t num_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (num_read <= 0) {
        perror("Failed to read from client");
        close(client_fd);
        return;
    }
    buffer[num_read] = '\0';

    char username[50], password[50], resource[50];
    sscanf(buffer, "%s %s %s", username, password, resource);

    if (verify_credentials(username, password)) {
        send_message(client_fd, "Authentication successful\n");

        // Check if the user has permission to access the resource
        if (check_permission(username, resource)) {
            // Process the request for the resource
            // For example, read from a file or query the database
            // Here, we assume the resource is a file
            int file_fd = open(resource, O_RDONLY);
            if (file_fd < 0) {
                perror("Failed to open resource");
                close(client_fd);
                return;
            }

            // Send the file descriptor to the client
            send_fd(client_fd, file_fd);
            close(file_fd);
        } else {
            send_message(client_fd, "Access denied\n");
        }
    } else {
        send_message(client_fd, "Authentication failed\n");
    }

    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    fd_set read_fds, master_fds;
    int fd_max;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    fd_max = server_fd;

    while (1) {
        read_fds = master_fds;
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) {
                    // New client connection
                    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
                        perror("accept error");
                    } else {
                        FD_SET(client_fd, &master_fds);
                        if (client_fd > fd_max) {
                            fd_max = client_fd;
                        }
                    }
                } else {
                    // Handle client request
                    handle_client(i);
                    FD_CLR(i, &master_fds);
                }
            }
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}


