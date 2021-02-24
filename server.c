//*****************************************************************
// File:   servidor.c
// Author: Javier Vivas Gomez
// Date:   Abril de 2020
// Coms:   Ejemplo de servidor mediante sockets
//        
//*****************************************************************


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
//Definimos 3 clientes como maximo
#define MAX_CLIENTS 3

//Definimos 1024 bytes como maximo
#define BUFFER_SZ 1024

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/* Estructura de direcciones propia del servidor  */
typedef struct{
	//Direccion
	struct sockaddr_in address;
	//Descriptor del socket
	int sockfd;
	//ID del struct
	int uid;
	//Puerto del cliente
	int puerto;
} client_est;

//Declaracion de vector de estructuras.
client_est *clients[MAX_CLIENTS];
//Inicializa el mutex, para garantizar la exclusion mutua durante el proceso de envio/recepcion
pthread_mutex_t mutex_cl = PTHREAD_MUTEX_INITIALIZER;

/* Funcion que detecta el "enter" en la cadena y le pone fin a la cadena */
void fin_cadena (char* cadena, int length) {
	int i;
	for (i = 0; i < length; i++) { 
		if (cadena[i] == '\n') {
		cadena[i] = '\0';
		break;
		}
	}
}



/* Eliminar clientes de la cola de clientes*/
void eliminarCola(int uid){
	//Le damos el control del mutex, garantizamos exclusion mutua
	pthread_mutex_lock(&mutex_cl);
	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				//Nos evita seguir recorreriendo inserviblemente el vector
				break;	
			}
		}
	}
	//Le quitamos el control del mutex, liberamos la exclusion mutua
	pthread_mutex_unlock(&mutex_cl);
}

/* Añadimos clientes a la cola de clientes*/
void AnyadirCola(client_est *cl){
	//Le damos el control del mutex, garantizamos exclusion mutua
	pthread_mutex_lock(&mutex_cl);
	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			//Nos evita seguir recorreriendo inserviblemente el vector
			break;
		}
	}
	//Le quitamos el control del mutex, liberamos la exclusion mutua
	pthread_mutex_unlock(&mutex_cl);
}

/* Envio de mensajes a todos los clientes */
void msg_env(char *s, int uid){
	pthread_mutex_lock(&mutex_cl);
	//
	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(write(clients[i]->sockfd, s, strlen(s)) < 0){
				perror("ERROR: write to descriptor failed");
				break;
			}	
		}
	}

	pthread_mutex_unlock(&mutex_cl);
}

/* Handle all communication with the client */
void *gestionaCliente(void *arg){
	
	//Declaracion del buffer de almacenamiento
	char buffer_salida[BUFFER_SZ];
	
	int flag_salida = 0;
	
	//aumentamos el contador de clientes atendiendose.
	cli_count++;
	
	client_est *cli = (client_est *)arg;
	
	int puertoAbierto = cli->puerto;
	char cadena[BUFFER_SZ];
	sprintf(cadena, "%d", puertoAbierto);
	fin_cadena(cadena, strlen(cadena));

	while(1){
		
		//Interrumpirá bucle while cuando sea flag_salida=1
		if (flag_salida) {
			break;
		}
		//Recibe mensaje del cliente y lo almacena en la variable buffer
		int recibido = recv(cli->sockfd, buffer_salida, BUFFER_SZ, 0);
		
		if (recibido > 0){
			if(strlen(buffer_salida) > 0){
				sprintf(cadena, "%d", puertoAbierto);
				strcat(cadena, " ");
				strcat(cadena,buffer_salida);

				msg_env(cadena, cli->uid);
				
				fin_cadena(cadena, strlen(cadena));
				fin_cadena(buffer_salida, strlen(buffer_salida));
				printf("%s\n",cadena);
				bzero(cadena, BUFFER_SZ);
			}
		} 
		else if (recibido == 0 || strcmp(buffer_salida, "exit") == 0){
			printf("%s", buffer_salida);
			msg_env(buffer_salida, cli->uid);
			flag_salida = 1;
		} 
		else {
			printf("Fallo: -1\n");
			flag_salida = 1;
		}
		//Restaura el buffer a 0 para los siguientes mensajes
		bzero(buffer_salida, BUFFER_SZ);
	}

	//Se cierra el socket del cliente
	close(cli->sockfd);
	//Se elimina cliente de la cola
	eliminarCola(cli->uid);
	//Liberamos el espacio asignado por malloc()
	free(cli);
	//Decrementamos el contador de clientes
	cli_count--;
	//pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	
	//Direccion de escucha del servidor.
	char *ip = "10.10.1.1";
	int port = 2018;
	int option = 1;
	int escucha_cliente = 0; 
	int conectar_cliente = 0;
	//Declaracion de la estructura de direcciones del server
	struct sockaddr_in s_addr;
	//Declaracion de la estructura de direcciones del cliente
	struct sockaddr_in c_addr;
	//Identificador del hilo del cliente
	pthread_t tid;

	/* Configuracion de opciones del socket */
	//Asignamos IPv4(AF_INET) y SOCK_STREAM(tcp/ip)
	escucha_cliente = socket(AF_INET, SOCK_STREAM, 0);
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr(ip);
	//Convertimos los bytes de Host a Bytes de red
	s_addr.sin_port = htons(port);


	if(setsockopt(escucha_cliente, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("Fallo: setsockopt failed");
		return EXIT_FAILURE;
	}

	/* Bind */
	if(bind(escucha_cliente, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0) {
		perror("Fallo: Bind socket fallido");
		return EXIT_FAILURE;
	}

	/* Escucha del servidor */
	if (listen(escucha_cliente, 10) < 0) {
		perror("Fallo: Escucha del socket fallida");
		return EXIT_FAILURE;
	}


	while(1){
		//Variable para almacenar el tamaño de la estructura de direcciones
		socklen_t clilen = sizeof(c_addr);
		//Aceptacion del cliente
		conectar_cliente = accept(escucha_cliente, (struct sockaddr*)&c_addr, &clilen);

		

		/* Las opciones del cliente */
		client_est *cli = (client_est *)malloc(sizeof(client_est));
		cli->address = c_addr;
		cli->sockfd = conectar_cliente;
		cli->uid = uid++;
		cli->puerto = c_addr.sin_port;

		//Añadimos el cliente a la cola 
		AnyadirCola(cli);
		//Creamos hilo del cliente
		pthread_create(&tid, NULL, &gestionaCliente, (void*)cli);

	}

	return EXIT_SUCCESS;
}
