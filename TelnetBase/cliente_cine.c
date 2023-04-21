#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ncurses.h>

#define PORT 12345 // Puerto del servidor
#define MAX_BUFFER_SIZE 2048 // Tamaño máximo del buffer de lectura/escritura

int main() {
    int socket_c = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_c == -1) {
        perror("Error al crear el socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in socket_addr;
    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &socket_addr.sin_addr) <= 0) {
        perror("Dirección IP no válida");
        return EXIT_FAILURE;
    }

    if (connect(socket_c, (struct sockaddr *)&socket_addr, sizeof(socket_addr)) == -1) {
        perror("Error al conectar al servidor");
        return EXIT_FAILURE;
    }

    fd_set read_fds;
    int max_fd = socket_c;
    char input[MAX_BUFFER_SIZE];
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(socket_c, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error en select");
            break;
        }

        if (FD_ISSET(socket_c, &read_fds)) {
            memset(input, 0, MAX_BUFFER_SIZE);
            ssize_t bytes_received = recv(socket_c, input, MAX_BUFFER_SIZE, 0);

            if (bytes_received == -1) {
                perror("Error al recibir datos del servidor");
                break;
            } else if (bytes_received == 0) {
                printf("Conexión cerrada por el servidor\n");
                break;
            }

            printf("%s", input);
            fflush(stdout);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(input, 0, MAX_BUFFER_SIZE);
            if (fgets(input, MAX_BUFFER_SIZE, stdin) == NULL) {
                break;
            }
            input[strcspn(input, "\n")] = '\0';
            ssize_t bytes_sent = send(socket_c, input, strlen(input), 0);

            if (bytes_sent == -1) {
                perror("Error al enviar datos al servidor");
                break;
            }
        }
    }
    close(socket_c);
    return EXIT_SUCCESS;
}
