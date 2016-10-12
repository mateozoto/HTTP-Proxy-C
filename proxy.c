/* CPSC 441 
   Mateo Zoto
   Assignment 1 - Proxy Server
   Program based of original echoserver.c
   
   Function of the Proxy Server serves to block all odd sized length content
   and to allow even sized content to be pushed to the user.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define PORTRANGE_MIN 8000
#define HTTP_HOSTNAME_MAX 64
//8kb buffer
#define BUFFER 8192
#define CONT_LEN 20

int g_signal = 1;
int *g_sock = NULL;

void signalHandler (int signal) {
	if (g_signal) {
		g_signal = 0; // setting the signal to zero breaks out of the while loop
		close(*g_sock); // closing socket unblocks the accept() function
	}
}
int main(int argc, char *argv[]){
	// Setup signal handler to exit parent process
	signal(SIGTERM, signalHandler);
	signal(SIGINT, signalHandler);
	
	//Variable declarations
	int inputport = 0;			// Port number read from input arguments
	int port;					// Port number
	
	char hostname[HTTP_HOSTNAME_MAX], sent[BUFFER], rec[BUFFER], contLen[CONT_LEN];
	char *incomingIP = (char*)malloc(100 * sizeof(char));

	int listenSocket;			// File descriptor for listening server socket
	int proxySocket;			// File descriptor for the proxy socket
	int activeSocket;			// File descriptor for the active socket

	
	int *sigSock = NULL;
	struct hostent *hp;
	
	struct sockaddr_in serverAdd;		
	struct sockaddr_in targetAdd;
	struct sockaddr_in clientAdd;

	socklen_t clientAddLen =  sizeof(clientAdd);

	// Read port from input arguments, or generate a random port
	if (argc > 1) {
		int inx = 0;
		while (argv[1][inx] != '\0') {
			if (argv[1][inx] > '9' || argv[1][inx] < '0') {
				printf("Invalid port!\n");
				return 0;
			}
			inputport = inputport*10 + (argv[1][inx] - '0');
			inx++;
		}
	}

	if (inputport >= PORTRANGE_MIN && 9999 <= 9999)
		port = inputport;
	else {
		printf("Generating random input port!\n");
		srand(time(NULL));
		port = rand()%1000 + PORTRANGE_MIN;
	}
	printf("Listening on port %d\n", port);

	// Initialize serverAdd struct
	serverAdd.sin_family = AF_INET;					// Set family
	serverAdd.sin_port = htons(port);				// Set port

	// Set the host address to any local server address (INADDR_ANY) using htonl (host to network long):
	serverAdd.sin_addr.s_addr = htonl(INADDR_ANY);
	// Or specify address using inet_pton:
	//inet_pton(AF_INET, "127.0.0.1", &serverAdd.sin_addr.s_addr); 
	targetAdd.sin_family = AF_INET;
	targetAdd.sin_port = htons(80);
	targetAdd.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Setup TCP listenSocket: (domain = AF_INET, type = SOCK_STREAM, protocol = 0)
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocket == -1){
		printf("Error creating socket\n");
		exit(1);
	}

	// Bind the listentSocket to the serverAdd's address and port
	if(bind(listenSocket, (struct sockaddr*) &serverAdd, sizeof(serverAdd)) == -1){
		printf("Bind error\n");
		exit(1);
	}

	// Listen for incoming connection requests. Limit the backlog to 5 (blocking)
	if(listen(listenSocket, 5) == -1){
		printf("Listen Error\n");
		exit(1);
	}
	
	while (0 == 0) { 
		// Accept an incoming connection, keep the incoming connection info in the proxy socket.
		proxySocket = accept(listenSocket, NULL, NULL);

		if (proxySocket == -1) {
			printf("Error accepting connection from socket...\n");
			exit(1);
		}
		
		//Figure out what is the IP and port of incoming connection
		inet_ntop(AF_INET, &clientAdd.sin_addr.s_addr, incomingIP, INET_ADDRSTRLEN);
		printf("Received connection from %s\n", incomingIP);
		
		if (fork() == 0)  {
			// This is the child process
			// The child process doesn't need the listener socket.
			close(listenSocket);		
			//Grab the target host information from the HTTP request.
			ssize_t receivedBytes = recv(proxySocket, sent, BUFFER, 0);
			//print out what the sent content
			printf("%s\n", sent);
			char from[BUFFER];
			strcpy(from, sent);
			char *to = strtok(from, "\r\n");
			struct hostent *hp;
			// traverse through the HTTP header for the hostname
			// method based off wordlengthclient.c
			while (to != NULL){
				//if token host isn't null
				if (strstr(to, "Host") != NULL){
					char *hostname = strtok(to, " ");\
					//everything after the empty space will be the hostname which we will gladly take
					hostname = strtok(NULL, " ");
					// convert the hostname for the struct
					hp = gethostbyname(hostname);
					// if the hostname from the header is null, we don't know what it is and we will just exit.
					if (hp == NULL){
						fprintf(stderr, "%s: is an unknown host so it will be exited.\n", hostname);
						// we don't want these connections so we will exit
						exit(1);
					}
					//eitherwise we will display to the server which host the user is trying to connect to.
					else
						fprintf(stderr, "%s has been connected to for filtering.\n", hostname);
					//add the host to the targetadd struct
					bcopy(hp->h_addr, &targetAdd.sin_addr, hp->h_length);
				}
				//rinse and repeat
				to = strtok(NULL, "\r\n");
			}
			//printf("Connected to target object client is trying to access... \n");
			activeSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (activeSocket == -1) {
				fprintf(stderr, "Client side: socket() failure...\n");
				exit(1);
			}
			if(connect(activeSocket, (struct sockaddr *)&targetAdd, sizeof(struct sockaddr_in)) == -1){	
				perror("Server side: Target connection failure...\n");
				exit(1);
			}
			ssize_t fdSentB = send(activeSocket, sent, receivedBytes, 0);
			ssize_t fdReadB = recv(activeSocket, rec, BUFFER, 0);
			//instantiation of the variables we will use to get content length
			char thing;
			char* cont = strstr(rec, "Content-Length:");
			int j = 0;
			
			//parse for the content length so we can determine what to do with the incoming content
			//based of:
			//http://stackoverflow.com/questions/8399129/c-beginner-string-parsing
			//traverse from 0 to 20. technically it can be 18, but 20 gives a better guarantee
			while (j < CONT_LEN) {
				thing = cont[strlen("Content-Length:")+j];
				if(thing == '\n' || thing == '\r') {
						break; //don't add \n or \r to the array
				}
				//as we traverse, we concatenate the string contents
				contLen[j] = thing;
				// increase the counter
				j++;
			}
			// convert the length to a variable
			int len = atoi(contLen);
			int isImage = 0;
			//Print out the HTTP requests and the like.
			printf("%s\n", rec);
			//keep looping through received content and block things as required
			//printf("outside read bytes\n");
			while (fdReadB > 0) {
				//printf("inside read bytes\n");
				//If it's an image, we will handle it differently then how we would handle other content such that 
				//the image will be replaced with another image.
				if (strstr(rec, "Content-Type: image/") != NULL){
					isImage = 1; //this is only done so it doesn't spam the console. all iterations are necessary though.
					//value always gets reset to 0 after the connection is done
					//odd
					if ((len % 2) != 0) {
						printf("This image has an odd content length, so it will be blocked. \n");
						fdSentB = send(proxySocket, "HTTP/1.1 302 Found\r\nLocation: http://pages.cpsc.ucalgary.ca/~carey/CPSC441/trollface.jpg\r\n\r\n", 96, 0);
						memset(&rec, 0, sizeof(rec));
						if (fdSentB < 0){
							printf("Error sending content back to client\n");
						}
					}
					//even
					else if ((len % 2 ) == 0 ) {
						printf("This image has an even content length, so its okay. \n");
						fdSentB = send(proxySocket, rec, fdReadB, 0);
						memset(&rec, 0, sizeof(rec));
						if (fdSentB < 0){
							printf("Error sending content back to client\n");
						}
					}
				}
				//Other content gets handled here i.e. an entire page of text.
				else{
					//odd
					if (((len % 2) != 0) && (isImage == 0)) {
						if (isImage == 0)
							printf("This content has an odd content length, so it will be blocked. \n");
						fdSentB = send(proxySocket, "HTTP/1.1 302 Found\r\nLocation: http://pages.cpsc.ucalgary.ca/~carey/CPSC441/errorpage.html\r\n\r\n", 92, 0);
						memset(&rec, 0, sizeof(rec));
						if (fdSentB < 0){
							printf("Error sending content back to client\n");
						}
					}
					//even
					else if ((len % 2 ) == 0 ) {
						if (isImage == 0)
							printf("This content has an even content length, so its okay. \n");
						fdSentB = send(proxySocket, rec, fdReadB, 0);
						memset(&rec, 0, sizeof(rec));
						if (fdSentB < 0){
							printf("Error sending content back to client\n");
						}
					}
				}
				// keep receiving if theres more to receive from the server
				fdReadB = recv(activeSocket, rec, BUFFER, 0);
			}
			// if there are no more bytes to be read, loop is done and the connection was closed.
			if (fdReadB == 0){
				printf("Connection has been closed.\n");
			}
			// there was an error receiving data
			else if (fdReadB < 0){
				printf("Error receiving data. \n");
			}
			close(proxySocket);	// The parent process doesn't need the child socket.
			exit(0);
		}
		close(proxySocket);	
	}
	close (listenSocket);
	return 0;
}