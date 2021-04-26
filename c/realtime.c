/* Real-time neural data filtering using C

The current iteration of this code is meant as a prototype to test if C
is a suitable language to use for the production system. Because speed is an
important part of this system, this program measures the round-trip latency
of each set of spike counts it sends, and saves the times to the output file
for further analysis.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


// TODO: Replace these eventually
#define HOST "127.0.0.1"
#define PORT 8889
#define N_NEURONS 5


// Probe mode
int probe_mode(char* host, int port) {

    printf("probe mode!\n");

    int spks[N_NEURONS] = {1, 2, 3, 4, 5};
    int filter_preds[N_NEURONS];

	int sock;
  	struct sockaddr_in server;

	// Create socket
  	sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock < 0) {
    	perror("Cannot create socket");
    	exit(1);
  	}

    // Connect to server
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = inet_addr(host);
  	server.sin_port = htons(port);
  	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    	perror("Cannot connect to server");
    	exit(2);
  	}

    // Send spike counts
    if (send(sock, &spks, N_NEURONS * sizeof(int), 0) < 0) {
        perror("Send failed");
        exit(3);
    }
  
    // Receive a reply from the server
    if (recv(sock, &filter_preds, N_NEURONS * sizeof(int), 0) < 0) {
        puts("recv failed");
        return 0;
    }
  
    // Print response
    printf("Server reply :\n");
    for (int i = 0; i < N_NEURONS; i++) {
        printf("%d\n", filter_preds[i]);
    }
  
    // close the socket
    close(sock);
    return 0;
}


// Processor mode
int processor_mode(char* host, int port) {

	int socket_desc, client_sock, c, read_size;
    struct sockaddr_in server, client;
    int message[N_NEURONS];
  
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
  
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
  
    // Bind the socket
    if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 1;
    }
  
    // Listen to the socket
    listen(socket_desc, 3);
  
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
  
    // accept connection from an incoming client
    client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);
  
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }
  
    puts("Connection accepted");
  
    // Receive a message from client
    while ((read_size = recv(client_sock, &message, N_NEURONS * sizeof(int), 0)) > 0) {
  
        write(client_sock, &message, N_NEURONS * sizeof(int));
    }
  
    if (read_size == 0) {
        puts("Client disconnected");
    }
    else if (read_size == -1) {
        perror("recv failed");
    }
 	return 0;
} 


int processor_mode_old(char* host, int port) {

    printf("processor mode!\n");

	/* Echo server code */

	int serverFd, clientFd;
  	struct sockaddr_in server, client;
  	int len;
  	//int port = 1234;
  	char buffer[1024];
  	//if (argc == 2) {
    //	port = atoi(argv[1]);
  	//}
  	serverFd = socket(AF_INET, SOCK_STREAM, 0);
  	if (serverFd < 0) {
    	perror("Cannot create socket");
    	exit(1);
  	}
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = INADDR_ANY;
  	server.sin_port = htons(port);
  	len = sizeof(server);
  	if (bind(serverFd, (struct sockaddr *)&server, len) < 0) {
    	perror("Cannot bind sokcet");
    	exit(2);
  	}
  	if (listen(serverFd, 10) < 0) {
    	perror("Listen error");
    	exit(3);
  	}
  	while (1) {
    	len = sizeof(client);
    	printf("waiting for clients\n");
    	if ((clientFd = accept(serverFd, (struct sockaddr *)&client, &len)) < 0) {
      		perror("accept error");
      		exit(4);
    	}
    	char *client_ip = inet_ntoa(client.sin_addr);
    	printf("Accepted new connection from a client %s:%d\n", client_ip, ntohs(client.sin_port));
    	memset(buffer, 0, sizeof(buffer));
    	int size = read(clientFd, buffer, sizeof(buffer));
    	if ( size < 0 ) {
      		perror("read error");
      		exit(5);
    	}
    	printf("received %s from client\n", buffer);
    	if (write(clientFd, buffer, size) < 0) {
      		perror("write error");
      		exit(6);
    	}
    	close(clientFd);
  	}

  close(serverFd);
  return 0;

}


// Print usage message
void print_usage() {

    printf("Usage: realtime [probe, processor]\n");

}

int main(int argc, char **argv) {

    // Parse mode ('probe' or 'processor') from argument list
    if (argc < 2) {
        print_usage();
        return 0;
    } 
    else {
        if (strcmp(argv[1], "probe") == 0) {
            return probe_mode(HOST, PORT);
        }
        else if (strcmp(argv[1], "processor") == 0) {
            return processor_mode(HOST, PORT);
        }
        else {
            print_usage();
            return 0;
        }
    }

    return 0;
}
