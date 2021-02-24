//*****************************************************************
// File:   cliente.c
// Author: Javier Vivas Gomez
// Date:   Abril de 2020
// Coms:   Ejemplo de cliente mediante sockets
//        
//*****************************************************************


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 4096

/* Variables Globales */
volatile sig_atomic_t flag = 0;
int sockfd = 0;


void fin_cadena (char* cadena, int length) {
	int i;
	for (i = 0; i < length; i++) { 
		if (cadena[i] == '\n') {
		cadena[i] = '\0';
		break;
		}
	}
}


void enviar() {
	char message[LENGTH] = {};
	char buffer[LENGTH] = {};

	while(1) {

		fgets(message, LENGTH, stdin);
		fin_cadena(message, LENGTH);

		if (strcmp(message, "exit") == 0) {
			break;
		} 
		else {
			sprintf(buffer, "%s\n", message);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(message, LENGTH);
		bzero(buffer, LENGTH);
	}
}

void recepcion() {
	char msg[LENGTH] = {};
	while (1) {
		int recibido = recv(sockfd, msg, LENGTH, 0);
		if (recibido > 0) {
			printf("%s", msg);
		} 
		else if (recibido == 0) {
			break;
		} 	
		else {
			// -1
		}
		memset(msg, 0, sizeof(msg));
	}
}

int main(int argc, char **argv){
	
	struct sockaddr_in ser_addr;      	//Declaracion de la estructura de direcciones
	pthread_t hiloEnvio;				//Declaracion del identificador del hilo de envio
	pthread_t hiloRecep;				//Declaracion del identificador del hilo de recepcion
	
	if(argc != 3){
		printf("Usage: %s <direccion IP> <puerto>\n", argv[0]);
		return EXIT_FAILURE;
	}
	int puerto = atoi(argv[2]);

	

	//Configuracion de la estructura de direcciones
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ser_addr.sin_family = AF_INET;					//AF_INET = IPv4
	ser_addr.sin_addr.s_addr = inet_addr(argv[1]);	//Campo correspondiente a la direccion IP que se le asigna.
	ser_addr.sin_port = htons(puerto);


	// Conexion al servidor
	int err = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}
	printf("...Conexion de pruebas prueba...\n");
	printf("...Estableciendo conexion con el servidor...\n");
	
	//Creacion y lanzamiento del hilo de envio 
	if(pthread_create(&hiloEnvio, NULL, (void *) enviar, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	//Creacion y lanzamiento del hilo de recepcion
	if(pthread_create(&hiloRecep, NULL, (void *) recepcion, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			printf("\nAdios\n");
			break;
		}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}
