#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOCKET_PATH "/tmp/trusted_daemon.sock"
#define BUFFER_SIZE 1024

// Function to receive a file descriptor from the server
int recv_fd(int socket) {
    struct msghdr msg = {0};
    struct iovec io;
    char buffer[1];
    io.iov_base = buffer;
    io.iov_len = 1;

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char cmsg_buffer[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cmsg_buffer;
    msg.msg_controllen = sizeof(cmsg_buffer);

    if (recvmsg(socket, &msg, 0) == -1) {
        perror("recvmsg");
        return -1;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg != NULL && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        int fd = *((int *)CMSG_DATA(cmsg));
        return fd; // Return the received file descriptor
    } else {
        fprintf(stderr, "No file descriptor received\n");
        return -1;
    }
}

int main() {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    char username[50], password[50], resource[50];
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    printf("Enter resource: ");
    scanf("%s", resource);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s %s %s", username, password, resource);

    if (write(sock_fd, buffer, strlen(buffer)) == -1) {
        perror("write");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Read the server's response
    ssize_t num_read = read(sock_fd, buffer, BUFFER_SIZE - 1);
    if (num_read <= 0) {
        perror("read");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buffer[num_read] = '\0';
    printf("Server response: %s\n", buffer);

    if (strstr(buffer, "Authentication successful") != NULL) {
        // Receive the file descriptor from the server
        int file_fd = recv_fd(sock_fd);
        if (file_fd == -1) {
            fprintf(stderr, "Failed to receive file descriptor\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }

        // Read the content of the file
        char file_buffer[BUFFER_SIZE];
        ssize_t file_read;
        while ((file_read = read(file_fd, file_buffer, BUFFER_SIZE)) > 0) {
            write(STDOUT_FILENO, file_buffer, file_read);
        }
        close(file_fd);
    }

    close(sock_fd);
    return 0;
}