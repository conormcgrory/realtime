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


// TODO: Figure out how to add HDF5 dependency
//#include "hdf5.h"



// TODO: Replace these with arguments
#define HOST "127.0.0.1"
#define PORT 8889
//#define IN_FPATH "../data/processed/s11_spks.h5"


// Code processor sends to probe to acknowledge header
const int ACK_CODE = 1;


// Probe mode
int probe_mode(char* host, int port) {


    // TODO: Replace this with HDF5 data
    const int n_neurons = 6;
    int spks[n_neurons] = {1, 2, 3, 4, 5, 6};


	// Create socket
  	int sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock < 0) {
    	perror("Cannot create socket");
    	return 1;
  	}

    // Connect to server
  	struct sockaddr_in server;
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = inet_addr(host);
  	server.sin_port = htons(port);
  	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    	perror("Cannot connect to server");
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

    // Send spike counts
    if (send(sock, &spks, n_neurons * sizeof(int), 0) < 0) {
        perror("Send failed");
        return 1;
    }
  
    // Memory for storing filter predictions
    double* filter_preds = (double*) malloc(n_neurons * sizeof(double));

    // Receive predictions from the server
    if (recv(sock, filter_preds, n_neurons * sizeof(double), 0) < 0) {
        perror("recv failed");
        free(filter_preds);
        return 1;
    }
  
    // Print response
    puts("Server reply:");
    for (int i = 0; i < n_neurons; i++) {
        printf("%f\n", filter_preds[i]);
    }

    // Free allocated memory
    free(filter_preds);
 
    // Close socket
    close(sock);

    return 0;
}


// Processor mode
int processor_mode(char* host, int port) {

    // Create socket
    int sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_desc < 0) {
        puts("Could not create socket");
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
    puts("Waiting for incoming connections...");
  
    // Accept connection from incoming client
    struct sockaddr_in client; 
    int c = sizeof(struct sockaddr_in);
    int sock_client = accept(sock_desc, (struct sockaddr*)&client, (socklen_t*)&c);
    if (sock_client < 0) {
        perror("accept failed");
        return 1;
    }
    puts("Connection accepted");

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

    // Array for storing spikes 
    int* spks_int = (int*) malloc(n_neurons * sizeof(int));

    // Array for storing spikes converted to doubles
    double* spks_double = (double*) malloc(n_neurons * sizeof(double));

    while(1) {

        // Recieve spikes from probe
        int read_size = recv(sock_client, spks_int, n_neurons * sizeof(int), 0);
        if (read_size == 0) {
            puts("Client disconnected");
            break;
        }
        else if (read_size == -1) {
            perror("recv failed");
            return 1;
        }

        // Convert spikes to double
        for (int i = 0; i < n_neurons; i++) {
            spks_double[i] = (double) spks_int[i];
        }

        // Echo message back to probe
        write(sock_client, spks_double, n_neurons * sizeof(double));
    }

    // Free memory
    free(spks_int);
    free(spks_double);

    // Close sockets
    close(sock_desc);
    close(sock_client);
    
    return 0;
} 


// Print usage message
void print_usage() {

    puts("Usage: realtime [probe, processor]");

}

int main(int argc, char **argv) {

    // TODO: Finish adding this!
    //hid_t file = H5Fopen (IN_FPATH, H5F_ACC_RDONLY, H5P_DEFAULT);
    //hid_t dset = H5Dopen (file, "spks", H5P_DEFAULT);

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
