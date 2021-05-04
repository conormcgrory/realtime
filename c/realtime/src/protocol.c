/* Protocol for communication between probe and processor */

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "protocol.h"


// Code processor sends to probe to acknowledge header
const int ACK_CODE = 1;


// Create TCP connection with processor
int probe_connect(char* host, int port, int n_neurons, struct ProbeConnection* conn) {

	// Create socket
  	int sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock < 0) {
    	perror("Cannot create socket");
    	return 1;
  	}

    // Connect to processsor
  	struct sockaddr_in server;
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = inet_addr(host);
  	server.sin_port = htons(port);
  	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    	perror("Cannot connect to processor");
    	return 1;
  	}

    // Send header
    if (send(sock, &n_neurons, sizeof(int), 0) < 0) {
        perror("Send failed");
        return 1;
    }

    // Receive ACK
    int hdr_resp;
    if (recv(sock, &hdr_resp, sizeof(int), 0) < 0) {
        perror("recv failed");
        return 1;
    }
    if (hdr_resp != ACK_CODE) {
        perror("Response to header not ACK");
        return 1;
    }

    // Populate struct
    conn->host = host;
    conn->port = port;
    conn->sock_id = sock;
    conn->n_neurons = n_neurons;
    conn->is_connected = 1;

    return 0;
}

// Close TCP connection with processor
int probe_disconnect(struct ProbeConnection* conn) {

    close(conn->sock_id);
    return 0;
}

// Send spikes across socket
int probe_send(struct ProbeConnection* conn, int* spks) {
    
    if (send(conn->sock_id, spks, conn->n_neurons * sizeof(int), 0) < 0) {
        perror("send failed");
        return 1;
    }

    return 0;
}

// Receive filter predictions from socket
int probe_recv(struct ProbeConnection* conn, double* fpreds) {
    
    if (recv(conn->sock_id, fpreds, conn->n_neurons * sizeof(double), 0) < 0) {
        perror("recv failed");
        return 1;
    }

    return 0;
}


/* Functions called by processor
 *
 * These functions are all called by the machine running in 'processor mode',
 * using a ProcessorConnection object.
 */

// Connect to probe
int processor_connect(char* host, int port, struct ProcessorConnection* conn) {

    // Create socket
    int sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_desc < 0) {
        fprintf(stderr, "Could not create socket\n");
        return 1;
    }
  
    // Bind socket
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (bind(sock_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return 1;
    }
  
    // Listen to the socket
    listen(sock_desc, 3);
  
    // Accept connection from incoming client
    struct sockaddr_in client; 
    int c = sizeof(struct sockaddr_in);
    int sock_client = accept(sock_desc, (struct sockaddr*)&client, (socklen_t*)&c);
    if (sock_client < 0) {
        perror("accept failed");
        return 1;
    }

    // Receive header
    int n_neurons;
    if (recv(sock_client, &n_neurons, sizeof(int), 0) < 0) {
        perror("recv failed");
        return 1;
    }

    // Send ACK
    if (send(sock_client, &ACK_CODE, sizeof(int), 0) < 0) {
        perror("Send failed");
        return 1;
    }

    // Populate struct
    conn->host = host;
    conn->port = port;
    conn->sock_desc_id = sock_desc;
    conn->sock_client_id = sock_client;
    conn->n_neurons = n_neurons;
    conn->is_connected = 1;

    return 0;
}

int processor_disconnect(struct ProcessorConnection* conn) {

    close(conn->sock_desc_id);
    close(conn->sock_client_id);

    return 0;
}


int processor_send(struct ProcessorConnection* conn, double* fpreds) {
    
    if (send(conn->sock_client_id, fpreds, conn->n_neurons * sizeof(double), 0) < 0) {
        perror("send failed");
        return 1;
    }
    
    return 0;
}


int processor_recv(struct ProcessorConnection* conn, int* spks) {
    
    int read_size = recv(conn->sock_client_id, spks, conn->n_neurons * sizeof(int), 0);
    if (read_size == 0) {
        conn->is_connected = 0;
    }
    else if (read_size == -1) {
        perror("recv failed");
        return 1;
    }

    return 0;
}
