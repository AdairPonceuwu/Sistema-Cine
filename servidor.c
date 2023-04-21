#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORT 12345 // Puerto del servidor
#define MAX_CONNECTIONS 5 // Máximo número de conexiones simultáneas
#define MAX_BUFFER_SIZE 1024 // Tamaño máximo del buffer de lectura/escritura

//Deinimos nuestros precios(Tomados de referencia de cinepolis)
#define PRICE_ADULT 75
#define PRICE_CHILD 65
#define PRICE_OLD 65
#define MAX_SEATS 10

//Molde para las salas de cine
typedef struct {
    int nSala;
    char pelicula[50];
    char horarios[4][10];
    int asientos[4][MAX_SEATS];
    int cupo[4];
} Sala;

//Variables para manejar los totales 
int num_adult = 0;
int num_children = 0;
int num_old = 0;
int last_cost_adult = 0;
int last_cost_children = 0;
int last_cost_old = 0;
int total = 0;

//Funciones prototipo
void process_purchase(Sala *salas_cine, int client_socket);
void ticket(int client_socket);

//Funcion para crear las 5 salas
void creando_salas(Sala *salas) {
    const char *peliculas[] = {
        "Burbuja",
        "SAO", 
        "Endgame", 
        "Shazam",
        "Valorant"
    };

    char matriz[5][4][10];
    strcpy(matriz[0][0], "10:00");
    strcpy(matriz[0][1], "12:30");
    strcpy(matriz[0][2], "15:00");
    strcpy(matriz[0][3], "18:30");

    strcpy(matriz[1][0], "11:50");
    strcpy(matriz[1][1], "13:30");
    strcpy(matriz[1][2], "16:00");
    strcpy(matriz[1][3], "19:15");

    strcpy(matriz[2][0], "22:15");
    strcpy(matriz[2][1], "19:20");
    strcpy(matriz[2][2], "16:30");
    strcpy(matriz[2][3], "14:00");

    strcpy(matriz[3][0], "21:00");
    strcpy(matriz[3][1], "18:00");
    strcpy(matriz[3][2], "15:30");
    strcpy(matriz[3][3], "12:25");

    strcpy(matriz[4][0], "20:45");
    strcpy(matriz[4][1], "17:25");
    strcpy(matriz[4][2], "13:30");
    strcpy(matriz[4][3], "11:30");
    
    for (int i = 0; i < 5; i++) {
        strcpy(salas[i].pelicula, peliculas[i]);
        //Numeramos las salas
        salas[i].nSala = i + 1;
        // Inicializar la disponibilidad de los asientos para cada horario
        for (int horario = 0; horario < 4; horario++) {
            for (int asiento = 0; asiento < 10; asiento++) {
                salas[i].asientos[horario][asiento] = 0; // 0 indica que el asiento está vacio.
            }
            salas[i].cupo[horario] = 10; // Inicializar la variable cupo para cada horario con el número de cupos disponibles
            strcpy(salas[i].horarios[horario], matriz[horario][i]); // Establece los horarios
            
        }
    }    
}
//Escoger opciones 
void format_menu(int client_socket, Sala *salas) {
    char buffer[MAX_BUFFER_SIZE];
    int op;
    int puente;
    while ((puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        buffer[puente] = '\0';
        sscanf(buffer, "%d", &op);
        switch (op) {
            case 1:
                process_purchase(salas, client_socket);
                break;
            case 2:
                //ticket(client_socket);
                break;
            case 3:
                snprintf(buffer, sizeof(buffer), "\n--------TICKET!--------\n");
                send(client_socket, buffer, strlen(buffer), 0);
                //ticket(client_socket);
                snprintf(buffer, sizeof(buffer), "--------Vuelva pronto!--------\n");
                send(client_socket, buffer, strlen(buffer), 0);
                return;
        }
    }
}

//Vender boleto
void sell_ticket(Sala *salas, int num_sala, int num_horario, int client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    int puente, num_asientos = 0, total_adulto = 0, total_chavos = 0, total_viejos = 0, precio_total = 0, num_adultos = 0, num_chavos = 0, num_viejos;
    Sala *sala = NULL;
    // Buscamos la sala
    for (int i = 0; i < 5; i++) {
        if (salas[i].nSala == num_sala) {
            sala = &salas[i];
            break;
        }
    }
    // Verifica que se haya encontrado la sala
    if (sala == NULL) {
        strcpy(buffer, "Digite el numero de una sala del 1-5\n");
        return;
    }
    // Verifica que el horario seleccionado tenga cupos disponibles
    if (sala->cupo[num_horario] <= 0) {
        strcpy(buffer, "No hay cupos disponibles para ese horario\n");
        return;
    }
 
    puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
    buffer[puente] = '\0';
    sscanf(buffer, "%d", &num_asientos);

    if (num_asientos <= 0) {
        strcpy(buffer, "Opcion invalida\n");
        return;
    }
    if (num_asientos > sala->cupo[num_horario]) {
        strcpy(buffer, "No hay cupos suficientes\n");
        return;
    }else{
        do{
            //Adult
            //strcat(buffer, "Asientos para adulto:\n");
            puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
            buffer[puente] = '\0';
            sscanf(buffer, "%d", &num_adultos);

            if (num_adultos < 0 || num_adultos > num_asientos) {
                strcpy(buffer, "Numero invalido de boletos para adultos\n");
                return;
            }
            //Children
            //strcat(buffer, "Asientos para niños:\n");
            puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
            buffer[puente] = '\0';
            sscanf(buffer, "%d", &num_chavos);
  
            if (num_chavos < 0 || num_chavos > num_asientos) {
                strcpy(buffer, "Numero invalido de boletos para niños\n");
                return;
            }
            //Old
            //strcat(buffer, "Asientos para adulto:\n");
            puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
            buffer[puente] = '\0';
            sscanf(buffer, "%d", &num_viejos);
            
            if (num_viejos < 0 || num_viejos > num_asientos) {
                strcpy(buffer, "Numero invalido de boletos para 3era edad\n");
                return;
            }

            if(num_adultos+num_chavos+num_viejos != num_asientos){
                strcpy(buffer, "Faltan boletos por ingresar\n");
            }
        }while(num_adultos+num_chavos+num_viejos != num_asientos);
        num_adult += num_adultos;
        num_children += num_chavos;
        num_old += num_viejos;
    }
    
    
    // Disponibilidad de asientos
    char buffer_a[MAX_BUFFER_SIZE] = "\0";
    for (int asiento = 0; asiento < 10; asiento++) {
            if (sala->asientos[num_horario][asiento] == 0) {
                char a[11];
                snprintf(a, sizeof(a), "└┘->%d  ", asiento+1);
                strcat(buffer_a,a);
            } else if (sala->asientos[num_horario][asiento] == 1) {
                char a[11];
                snprintf(a,sizeof(a), "└┘->/  ");
                strcat(buffer_a,a);
            }
    }
    strcat(buffer_a, "\n");
    send(client_socket, buffer_a, strlen(buffer_a), 0);

    char buffer_asiento[MAX_BUFFER_SIZE] = "\0";
    for (int i = 0; i < num_asientos; i++) {
        int asiento = 0;
        puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
        buffer[puente] = '\0';
        sscanf(buffer, "%d", &asiento);

        if (asiento < 0 || asiento > 10 || sala->asientos[num_horario][asiento-1] == 1){
            char b[20];
            snprintf(b,sizeof(b), "Escoga otro asiento");
            strcat(buffer_asiento,b);
            i--;
        } else {
            char b[20];
            snprintf(buffer_asiento,sizeof(b),"%s", "Asiento confirmado");
            strcat(buffer_asiento,b);
            asiento--;   
            sala->asientos[num_horario][asiento] = 1;
        }
        send(client_socket, buffer_asiento, strlen(buffer_asiento), 0);
    }

    // Decrementa el número de cupos disponibles para el horario seleccionado
    sala->cupo[num_horario] -= num_asientos;

    // Total a pagar
    total_adulto = num_adultos * PRICE_ADULT;
    total_chavos = num_chavos * PRICE_CHILD;
    total_viejos = num_viejos * PRICE_OLD;
    last_cost_adult += total_adulto;
    last_cost_children += total_chavos;
    last_cost_old += total_viejos;
    precio_total = total_adulto + total_chavos + total_viejos;
    total += precio_total;

    // Imprime los detalles de la compra
    snprintf(buffer, sizeof(buffer), "\nDetalles de la compra:\nSala: %d\nPelicula: %s\nHorario: %s\nNumero de asientos: %d\nNumero de boletos para adultos: %d\nNumero de boletos para niños: %d\nNumero de boletos para 3era edad: %d\nPrecio total: $%d\n", num_sala, sala->pelicula, sala->horarios[num_horario], num_asientos, num_adultos, num_chavos,num_viejos,precio_total);
    send(client_socket, buffer, strlen(buffer), 0);

}

//Proceso de compra del ticket
void process_purchase(Sala *salas, int client_socket) {
    int seleccion = 0, numSala = 0, puente, s_horario = 0;
    char buffer[MAX_BUFFER_SIZE];

    //Mostrar peliculas al cliente
    char name_movie[MAX_BUFFER_SIZE] = "¡Estrenos!:\n";
    for (int i = 0; i < 5; i++) {
        char movie[50];
        snprintf(movie, sizeof(movie), "%d->%s\n", i+1, salas[i].pelicula);
        strcat(name_movie, movie);
    }
    send(client_socket, name_movie, strlen(name_movie), 0);

    //Recibir respuesta del cliente para la pelicula
    puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
    buffer[puente] = '\0';
    sscanf(buffer, "%d", &seleccion);

    if (seleccion < 1 || seleccion > 5) {
        strcpy(buffer, "Opcion invalida\n");
        return;
    }

    //Mostrar horarios al cliente
    char horarios[MAX_BUFFER_SIZE] = "Horarios:\n";
    for (int i = 0; i < 4; i++)
    {
        char opciones[100];
        snprintf(opciones, sizeof(opciones), "%d->%s->Asientos libres:%d\n", i+1, salas[seleccion-1].horarios[i], salas[seleccion-1].cupo[i]);
        strcat(horarios, opciones);
    }
    send(client_socket, horarios, strlen(horarios), 0);
    
    numSala = salas[seleccion-1].nSala;
    //Recibir respuesta del cliente para el horario
    puente = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
    buffer[puente] = '\0';
    sscanf(buffer, "%d", &s_horario);

    if (s_horario < 1 || s_horario > 4) {
        strcpy(buffer, "Opcion invalida\n");
        return;
    }

    sell_ticket(salas, numSala, s_horario - 1, client_socket);
    if (numSala == 0) {
        strcpy(buffer, "No hay salas disponibles para esa pelicula\n");
        return;
    }
}

//Funcion principal
int main (int argc, char *argv[])
{
    Sala salas[5];
    creando_salas(salas);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // Crear socket del servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error al crear el socket del servidor");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Asociar el socket con la dirección del servidor
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al asociar el socket con la dirección del servidor");
        exit(1);
    }

    // Escuchar por conexiones entrantes
    if (listen(server_socket, MAX_CONNECTIONS) == -1) {
        perror("Error al escuchar por conexiones entrantes");
        exit(1);
    }

    printf("Servidor del cine en funcionamiento...\n");
    printf("Servidor funcionando, puerto: %d\n", PORT);

    while (1) {

        // Aceptar una conexión entrante
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error al aceptar la conexión entrante");
            exit(1);
        }

        printf("Cliente detectado: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        format_menu(client_socket, salas);
    }

    // Cierra el socket del servidor
    close(server_socket);

    return 0;
}

