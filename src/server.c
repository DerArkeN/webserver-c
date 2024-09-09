#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "consts.h"

const int BUFFER_SIZE = 256;

// response should be free'd after sending
char *build_response(const char *status, const char *content_type, char *body) {
    const char *format = "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n%s";
    char *resp = malloc(
        strlen(format) + strlen(status) + strlen(content_type) + sizeof(size_t) + strlen(body));
    sprintf(resp, format, status, content_type, strlen(body), body);
    return resp;
}

int handle_request(int client_fd, char *request) {
    char *method = strtok(request, " ");
    char *path = strtok(NULL, " ");

    printf("%s request made to %s\n", method, path);

    int send_status;
    char* resp;

    if (strcmp(method, METHOD_GET) != 0) {
        resp = build_response(ST_METHOD_NOT_ALLOWED_405, CT_TEXT_PLAIN, "");
        send_status = send(client_fd, resp, strlen(resp), 0);
    }

    char *path_copy = path;
    char *path_1 = strtok(path_copy, "/");
    char *path_2 = strtok(NULL, "/");

    if (strcmp(path, "/") == 0) {
        resp = build_response(ST_OK_200, CT_TEXT_PLAIN, "Welcome!") ;
        send_status = send(client_fd, resp, strlen(resp), 0);
    } else if (strcmp(path_1, "echo") == 0) {
        resp = build_response(ST_OK_200, CT_TEXT_PLAIN, path_2);
        send_status = send(client_fd, resp, strlen(resp), 0);
    } else {
        resp = build_response(ST_NOT_FOUND_400, CT_TEXT_PLAIN, (char*) ST_NOT_FOUND_400);
        send_status = send(client_fd, resp, strlen(resp), 0);
    }

    free(resp);

    if (send_status == -1) {
        printf("Sending failed: %s...\n", strerror(errno));
        return -1;
    }

    return 0;
}

void *handle_client(void *arg) {
    printf("Client connected\n");

    int client_fd = *((int *) arg);

    char request[BUFFER_SIZE];
    if (recv(client_fd, request, BUFFER_SIZE, 0) == -1) {
        printf("Receiving failed: %s...\n", strerror(errno));
        return NULL;
    }

    handle_request(client_fd, request);

    close(client_fd);
    free(arg);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4221),
        .sin_addr = { .s_addr = htonl(INADDR_ANY) },
    };

    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Waiting for a client to connect...\n");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    do {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (*client_fd == -1) {
            printf("Accepting failed: %s...", strerror(errno));
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *) client_fd);
    } while(1);

    close(server_fd);

    return 0;
}

