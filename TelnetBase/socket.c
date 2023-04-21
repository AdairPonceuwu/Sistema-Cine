#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PORT 12345 // Puerto del servidor
#define MAX_CONNECTIONS 5 // Máximo número de conexiones simultáneas
#define MAX_BUFFER_SIZE 256 // Tamaño máximo del buffer de lectura/escritura

//Deinimos nuestros precios(Tomados de referencia de cinepolis)
#define PRICE_ADULT 75
#define PRICE_CHILD 65
#define PRICE_OLD 65
#define MAX_SEATS 10


//Estructura para el ticket
typedef struct{
    int num_sala;
    char pelicula[50];
    char horario[50];
    int num_asientos;
    int num_adultos;
    int num_chavos;
    int num_viejos;
    int precio_total;
} Ticket;

//estructura para representar una sala de cine
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
int num_tickets = 0;
int num_archivo = 0;
char arreglo[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};


//funciones prototipo
void process_purchase(Sala *salas, int client_socket, Ticket *tickets);
void ticket_impress(Ticket *tickets, int client_socket);
void save_ticket(Ticket *tickets);


//funciones para manejar solicitudes del cliente
void format_menu(int client_socket, Sala *salas, Ticket *tickets) {
    char buffer[MAX_BUFFER_SIZE];
    int op;
    while (1) {
        memset(buffer, 0, MAX_BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer),
                 "\n--- CINE LA CINE ---\n"
                 "1.Comprar entradas\n"
                 "2.Consultar total de compra\n"
                 "3.Salir \n"
                 "**Precio de boletos:**\n"
                 "  Adultos $75 y Niños/3era edad $65\n"
                 "Elige la opcion a consultar: \n");
        send(client_socket, buffer, strlen(buffer), 0);

        memset(buffer, 0, MAX_BUFFER_SIZE);
        if (recv(client_socket, buffer, MAX_BUFFER_SIZE, 0) <= 0) {
            break;
        }
        sscanf(buffer, "%d", &op);

        if (op < 1 || op > 3) {
            snprintf(buffer, sizeof(buffer), "Escoga una opcion del 1-3\n");
            send(client_socket, buffer, strlen(buffer), 0);
            continue;
        }
        switch (op) {
            case 1:
                process_purchase(salas, client_socket,tickets);
                break;
            case 2:
                snprintf(buffer, sizeof(buffer), "\n--------TICKETS!--------\n");
                send(client_socket, buffer, strlen(buffer), 0);
                ticket_impress(tickets,client_socket);
                break;
            case 3:
                snprintf(buffer, sizeof(buffer), "\n--------TICKET IMPRESO!--------\n");
                send(client_socket, buffer, strlen(buffer), 0);
                save_ticket(tickets);
                snprintf(buffer, sizeof(buffer), "--------Vuelva pronto!--------\n");
                send(client_socket, buffer, strlen(buffer), 0);
		for(int i=0; i<num_tickets;i++){
			memset(&tickets[i], 0, sizeof(Ticket));	
		}
		num_tickets = 0;
                close(client_socket);
		break;
        }
    }
}

//inicializacion de salas con sus respectivos atributos
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
        //asigna el valor a cada sala
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

void venderBoletosHorario(Sala *salas, int num_sala, int num_horario, int client_socket, Ticket *tickets) {
    char buffer[MAX_BUFFER_SIZE];
    int num_asientos = 0, total_adulto = 0, total_chavos = 0, total_viejos = 0, precio_total = 0, num_adultos = 0, num_chavos = 0, num_viejos;
    Sala *sala = NULL;
    //Buscamos la sala
    for (int i = 0; i < 5; i++) {
        if (salas[i].nSala == num_sala) {
            sala = &salas[i];
            break;
        }
    }
    // Verifica que se haya encontrado la sala
    if (sala == NULL) {
        snprintf(buffer, sizeof(buffer), "Digite el numero de una sala del 1-5\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }
    // Verifica que el horario seleccionado tenga cupos disponibles
    if (sala->cupo[num_horario] <= 0) {
        snprintf(buffer, sizeof(buffer), "No hay cupos disponibles para ese horario\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    // Disponibilidad de asientos
    snprintf(buffer, sizeof(buffer), "Asientos disponibles:\n");
    send(client_socket, buffer, strlen(buffer), 0);

    for (int asiento = 0; asiento < 10; asiento++) {
        memset(buffer, 0, MAX_BUFFER_SIZE);
            if (sala->asientos[num_horario][asiento] == 0) {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "└┘->%d ", asiento+1);
            } else if (sala->asientos[num_horario][asiento] == 1) {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "└┘->X ");
            }
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " ");
        send(client_socket, buffer, strlen(buffer), 0);
    }

    // Solicita al cliente que seleccione un número de asientos
    snprintf(buffer, sizeof(buffer), "\nDigite cantidad de asientos a comprar: ");
    send(client_socket, buffer, strlen(buffer), 0);
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
        return;
    }
    sscanf(buffer, "%d", &num_asientos);
    if (num_asientos <= 0) {
        snprintf(buffer, sizeof(buffer), "Digite un numero correcto\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }
    // Verifica que haya suficientes cupos disponibles para el número de asientos solicitado
    if (num_asientos > sala->cupo[num_horario]) {
        snprintf(buffer, sizeof(buffer), "No hay cantidad de asientos suficiente\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }else{
        do{
             //Adult
            snprintf(buffer, sizeof(buffer), "Ingrese el numero de boletos para adultos: ");
            send(client_socket, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
                return;
            }
            sscanf(buffer, "%d", &num_adultos);
            if (num_adultos < 0 || num_adultos > num_asientos) {
                snprintf(buffer, sizeof(buffer), "Numero invalido de boletos para adultos\n");
                send(client_socket, buffer, strlen(buffer), 0);
                return;
            }

            //Children
            snprintf(buffer, sizeof(buffer), "Ingrese el numero de boletos para niños: ");
            send(client_socket, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
                return;
            }
            sscanf(buffer, "%d", &num_chavos);
            if (num_chavos < 0 || num_chavos > num_asientos) {
                snprintf(buffer, sizeof(buffer), "Numero invalido de boletos para niños\n");
                send(client_socket, buffer, strlen(buffer), 0);
            return;
            }
            //Old
            snprintf(buffer, sizeof(buffer), "Ingrese el numero de boletos para 3era edad: ");
            send(client_socket, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
                return;
            }
            sscanf(buffer, "%d", &num_viejos);
            
            if (num_viejos < 0 || num_viejos > num_asientos) {
                snprintf(buffer, sizeof(buffer), "Numero invalido de boletos para 3era edad\n");
                send(client_socket, buffer, strlen(buffer), 0);
                return;
            }

            if(num_adultos+num_chavos+num_viejos != num_asientos){
                snprintf(buffer, sizeof(buffer), "Faltan o exceden los asientos que ha ingresado\n");
                send(client_socket, buffer, strlen(buffer), 0);
            }
        }while(num_adultos+num_chavos+num_viejos != num_asientos);
        num_adult += num_adultos;
        num_children += num_chavos;
        num_old += num_viejos;
    }

    // Registra los asientos seleccionados
    for (int i = 0; i < num_asientos; i++) {
        int asiento = 0;
        snprintf(buffer, sizeof(buffer), "Ingrese el número del asiento %d: \n", i + 1);
        send(client_socket, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
        if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
            return;
        }
        sscanf(buffer, "%d", &asiento);
        asiento--;
        if (asiento < 0 || asiento > 10 || sala->asientos[num_horario][asiento] == 1) {
            snprintf(buffer, sizeof(buffer), "Invalido\n");
            send(client_socket, buffer, strlen(buffer), 0);
            i--;
        } else {
            sala->asientos[num_horario][asiento] = 1;
        }
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


    //Ticket
    tickets[num_tickets].num_sala = num_sala;
    strcpy(tickets[num_tickets].horario, sala->horarios[num_horario]);
    strcpy(tickets[num_tickets].pelicula, sala->pelicula);
    tickets[num_tickets].num_asientos = num_asientos;
    tickets[num_tickets].num_adultos = num_adultos;
    tickets[num_tickets].num_chavos = num_chavos;
    tickets[num_tickets].num_viejos = num_viejos;
    tickets[num_tickets].precio_total = precio_total;
    num_tickets++;

    // Imprime los detalles de la compra
    snprintf(buffer, sizeof(buffer), "\nCompra:\nSala: %d\nPelicula: %s\nHorario: %s\nNumero de asientos: %d\nNumero de boletos para adultos: %d\nNumero de boletos para niños: %d\nNumero de boletos para 3era edad: %d\nPrecio total: $%d\n", num_sala, sala->pelicula, sala->horarios[num_horario], num_asientos, num_adultos, num_chavos,num_viejos,precio_total);
    send(client_socket, buffer, strlen(buffer), 0);
}


//funcion para seleccionar pelicula
void process_purchase(Sala *salas, int client_socket, Ticket *tickets) {
    int seleccion = 0, numSala = 0;
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "Seleccione una pelicula:\n");
    send(client_socket, buffer, strlen(buffer), 0);

    for (int i = 0; i < 5; i++) {
        snprintf(buffer, sizeof(buffer), "%d->%s\n", i+1, salas[i].pelicula);
        send(client_socket, buffer, strlen(buffer), 0);
    }

    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
        return;
    }
    sscanf(buffer, "%d", &seleccion);


    if (seleccion < 1 || seleccion > 5) {
        snprintf(buffer, sizeof(buffer), "Digite una sala de 1-5\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }

    
    for (int i = 0; i < 5; i++) {
        if (strcmp(salas[i].pelicula, salas[seleccion-1].pelicula) == 0) {
            snprintf(buffer, sizeof(buffer), "Sala %d - %s:\n", salas[i].nSala, salas[i].pelicula);
            send(client_socket, buffer, strlen(buffer), 0);
            for (int horario = 0; horario < 4; horario++) {
                snprintf(buffer, sizeof(buffer), "%d->%s->Asientos libres:%d\n", horario+1, salas[i].horarios[horario], salas[i].cupo[horario]);
                send(client_socket, buffer, strlen(buffer), 0);
            }

            numSala = salas[i].nSala;
	    snprintf(buffer, sizeof(buffer), "Seleccione un horario:\n");
            send(client_socket, buffer, strlen(buffer), 0);

            int seleccion_horario = 0;
            memset(buffer, 0, sizeof(buffer));
            if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
                return;
            }
            sscanf(buffer, "%d", &seleccion_horario);


            if (seleccion_horario < 1 || seleccion_horario > 4) {
                snprintf(buffer, sizeof(buffer), "Seleccione una opcion del 1-4\n");
                send(client_socket, buffer, strlen(buffer), 0);
                return;
            }
            venderBoletosHorario(salas, numSala, seleccion_horario - 1, client_socket, tickets);
        }
    }
    if (numSala == 0) {
        snprintf(buffer, sizeof(buffer), "No hay salas disponibles para esa pelicula\n");
        send(client_socket, buffer, strlen(buffer), 0);
        return;
    }
}

//Ticket, resumen de compra
void ticket_impress(Ticket *tickets, int client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    for (int i=0; i<num_tickets;i++){
	snprintf(buffer, sizeof(buffer), "\n---Compra número:%d---\n", i+1);
	send(client_socket, buffer, strlen(buffer), 0);
	snprintf(buffer, sizeof(buffer), "Numero de sala:%d\n", tickets[i].num_sala);
	send(client_socket, buffer, strlen(buffer), 0);
	snprintf(buffer, sizeof(buffer), "Pelicula:%s\n", tickets[i].pelicula);
	send(client_socket, buffer, strlen(buffer), 0);
	snprintf(buffer, sizeof(buffer), "Horario:%s\n", tickets[i].horario);
	send(client_socket, buffer, strlen(buffer), 0);
	snprintf(buffer, sizeof(buffer), "Numeros de asientos:%d\n", tickets[i].num_asientos);
	send(client_socket, buffer, strlen(buffer), 0);
        snprintf(buffer, sizeof(buffer), "Numeros de asientos para adultos:%d\n", tickets[i].num_adultos);
	send(client_socket, buffer, strlen(buffer), 0);
	snprintf(buffer, sizeof(buffer), "Numeros de asientos para niño:%d\n", tickets[i].num_chavos);
	send(client_socket, buffer, strlen(buffer), 0);
	snprintf(buffer, sizeof(buffer), "Numeros de asientos para 3era edad:%d\n", tickets[i].num_viejos);
	send(client_socket, buffer, strlen(buffer), 0);
        snprintf(buffer, sizeof(buffer), "Total:$%d\n", tickets[i].precio_total);
	send(client_socket, buffer, strlen(buffer), 0);
    }
}

//Guardar en un archivo
void save_ticket(Ticket *tickets){
    char nombre_archivo[100];
    char a[2];
    a[0] = arreglo[num_archivo];
    strcpy(nombre_archivo, "Ticket");
    strcat(nombre_archivo, a);
    strcat(nombre_archivo, ".txt");
    int fd;
    fd = open(nombre_archivo,O_WRONLY|O_CREAT,0666);
    //STRING
    char cadena1[20];
    char cadena2[20];
    char cadena3[20];
    char buffer[MAX_BUFFER_SIZE];
    for (int i=0; i<num_tickets;i++){
	snprintf(buffer, sizeof(buffer), "\n---Compra número:%d---\n", i+1);
	write(fd,buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer), "Numero de sala:%d\n", tickets[i].num_sala);
	write(fd,buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer), "Pelicula:%s\n", tickets[i].pelicula);
	write(fd,buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer), "Horario:%s\n", tickets[i].horario);
	write(fd,buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer), "Numeros de asientos:%d\n", tickets[i].num_asientos);
	write(fd,buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "Numeros de asientos para adultos:%d\n", tickets[i].num_adultos);
	write(fd,buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer), "Numeros de asientos para niño:%d\n", tickets[i].num_chavos);
	write(fd,buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer), "Numeros de asientos para 3era edad:%d\n", tickets[i].num_viejos);
	write(fd,buffer, strlen(buffer));
        snprintf(buffer, sizeof(buffer), "Total:$%d\n", tickets[i].precio_total);
	write(fd,buffer, strlen(buffer));
    }
    //CERRAR ARCHIVO
    close(fd);
    num_archivo++;
}

int main() {

    Sala salas[5];
    creando_salas(salas);
    Ticket tickets[10];
    
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

    printf("Server ready...\n");
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

        format_menu(client_socket, salas, tickets);
    }

    // Cierra el socket del servidor
    close(server_socket);

    return 0;
}