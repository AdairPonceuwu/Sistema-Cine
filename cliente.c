#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORT 12345 // Puerto del servidor
#define MAX_BUFFER_SIZE 1024 // Tamaño máximo del buffer de lectura/escritura

int menu(){
    int op;
    printf(
        "\n--- CINE LA CINE ---\n"
        "1.Comprar entradas\n"
        "2.Consultar total de compra\n"
        "3.Salir \n"
        "**Precio de boletos:**\n"
        "  Adultos $75 y Niños/3era edad $65\n"
        "Porfavor, escoge una opcion: \n");
    scanf("%d", &op);
    return op;
}

int main()
{
    int socket_c;
    struct sockaddr_in socket_addr;
    char buffer[MAX_BUFFER_SIZE];
    
    socket_c = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_c < 0)
    {
        perror("Socket no creado");
        return 1;
    }

    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(PORT);
    socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(socket_c, (struct sockaddr *)&socket_addr, sizeof(socket_addr)) < 0)
    {
        perror("Error al realizar la conexion");
        return 1;
    }

    printf("Servidor: %s:%d\n", inet_ntoa(socket_addr.sin_addr), ntohs(socket_addr.sin_port));

    while (1)
    {
        int op = 0;
        op = menu();

        snprintf(buffer, MAX_BUFFER_SIZE, "%d", op);
        send(socket_c, buffer, strlen(buffer), 0);

        int puente = recv(socket_c, buffer, MAX_BUFFER_SIZE, 0);
        buffer[puente] = '\0';
        printf("%s", buffer);

        //Realizar la compra
        if (op == 1)
        {
            int n_pelicula, n_horario, n_asientos, n_asientos_n, n_asientos_a, n_asientos_v, n_asiento;

            //Peliculas
            printf("Número de la película deseada: ");
            scanf("%d", &n_pelicula);
            snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_pelicula);
            send(socket_c, buffer, strlen(buffer), 0);

            puente = recv(socket_c, buffer, MAX_BUFFER_SIZE, 0);
            buffer[puente] = '\0';
            printf("%s", buffer);

            //Horarios
            printf("Número del horario deseado: ");
            scanf("%d", &n_horario);
            snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_horario);
            send(socket_c, buffer, strlen(buffer), 0);

            puente = recv(socket_c, buffer, MAX_BUFFER_SIZE, 0);
            buffer[puente] = '\0';
            printf("%s", buffer);

            //Asientos
            printf("Cantidad de asientos a escoger: ");
            scanf("%d", &n_asientos);
            snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_asientos);
            send(socket_c, buffer, strlen(buffer), 0);

            //Asientos adultos
            printf("Cantidad de asientos para adultos: ");
            scanf("%d", &n_asientos_a);
            snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_asientos_a);
            send(socket_c, buffer, strlen(buffer), 0);            
            //Asientos niños
            printf("Cantidad de asientos para niños: ");
            scanf("%d", &n_asientos_n);
            snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_asientos_n);
            send(socket_c, buffer, strlen(buffer), 0);
            //Asientos viejos
            printf("Cantidad de asientos para 3era edad: ");
            scanf("%d", &n_asientos_v);
            snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_asientos_v);
            send(socket_c, buffer, strlen(buffer), 0);
            for(int i=0;i<n_asientos;i++){
                //Digite la cantidad de asientos
                printf("Esperando n.asiento: ");
                scanf("%d", &n_asiento);
                snprintf(buffer, MAX_BUFFER_SIZE, "%d", n_asiento);
                send(socket_c, buffer, strlen(buffer), 0);
                puente = recv(socket_c, buffer, MAX_BUFFER_SIZE, 0);
                buffer[puente] = '\0';
                printf("%s", buffer);
            }
        }

        //Finalizar el sistema
        if (op == 3)
        {
            break;
        }
    }

    close(socket_c);
    return 0;
}