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
#include <sys/time.h>
#include "hdf5.h"



// TODO: Replace these with arguments
#define HOST "127.0.0.1"
#define PORT 8889
#define IN_FPATH "../../data/processed/test_spks_clang.h5"
#define OUT_FPATH "../../data/results/c_echo.h5"


// Code processor sends to probe to acknowledge header
const int ACK_CODE = 1;


// Get dimensions (num. time points, num. neurons) from input data
int get_data_dims(char* in_fpath, int* n_pts, int* n_neurons) {

    // Default values
    *n_pts = 0;
    *n_neurons = 0;

    // Open file
    hid_t file = H5Fopen(in_fpath, H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, "spks", H5P_DEFAULT);
    hid_t dspace = H5Dget_space(dset);

    // Get dimensions
    hsize_t data_dims[2];
    int ndims = H5Sget_simple_extent_dims(dspace, data_dims, NULL);
    if (ndims != 2) {
        fprintf(stderr, "Input data not two-dimensional\n");
        return 1;
    }

    // Write return values
    *n_pts = data_dims[0];
    *n_neurons = data_dims[1];

    // Close file
    H5Sclose(dspace);
    H5Dclose(dset);
    H5Fclose(file);

    return 0;
}


// Load spike counts from HDF5 file
int load_data(char* in_fpath, int* data) {

    // Open file
    hid_t file = H5Fopen(in_fpath, H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, "spks", H5P_DEFAULT);

    int status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (status != 0) {
        fprintf(stderr, "Failed to read input data\n");
        return 1;
    }

    // Close file
    H5Dclose(dset);
    H5Fclose(file);

    return 0;
}


// Write filter predictions and round-trip times to HDF5 file
int save_data(char* out_fpath, double* fpreds, double* rt_times, int n_pts, int n_neurons) {

    // Create HDF5 file
    hid_t file = H5Fcreate(out_fpath, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Create dataspace for filter predictions
    hsize_t dims_fp[2]; 
    dims_fp[0] = n_pts;
    dims_fp[1] = n_neurons;
    hid_t dspace_fp = H5Screate_simple(2, dims_fp, NULL);

    // Create dataset for filter predictions
    hid_t dset_fp = H5Dcreate(file, "filter_preds", H5T_IEEE_F64LE, dspace_fp, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    
    // Write filter predictions to file
    int status_fp = H5Dwrite(dset_fp, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, fpreds);
    if (status_fp != 0) {
        fprintf(stderr, "Failed to write output data\n");
        return 1;
    }

    // Close dataspace and dataset
    H5Sclose(dspace_fp);
    H5Dclose(dset_fp);

    // Create dataspace for round-trip times
    hsize_t dims_rt[1]; 
    dims_rt[0] = n_pts;
    hid_t dspace_rt = H5Screate_simple(1, dims_rt, NULL);

    // Create dataset for round-trip times
    hid_t dset_rt = H5Dcreate(file, "rt_times_us", H5T_IEEE_F64LE, dspace_rt, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    
    // Write round-trip times to file
    int status_rt = H5Dwrite(dset_rt, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rt_times);
    if (status_rt != 0) {
        fprintf(stderr, "Failed to write output data\n");
        return 1;
    }

    // Close dataspace and dataset
    H5Sclose(dspace_rt);
    H5Dclose(dset_rt);

    // Close file
    H5Fclose(file);

    return 0;
}


// Probe mode
int probe_mode(char* host, int port) {

    // Read number of data points and number of neurons from input file
    int n_pts, n_neurons;
    get_data_dims(IN_FPATH, &n_pts, &n_neurons);

    // Allocate array for spike counts
    int** spks = (int **) malloc(n_pts * sizeof(int *));
    spks[0] = (int *) malloc(n_pts * n_neurons * sizeof(int));
    for (int i = 1; i < n_pts; i++) {
        spks[i] = spks[0] + i * n_neurons;
    }

    // Load spike counts into memory
    load_data(IN_FPATH, spks[0]);


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

    
    // Allocate array for filter predictions
    double** filter_preds = (double **) malloc(n_pts * sizeof(double *));
    filter_preds[0] = (double *) malloc(n_pts * n_neurons * sizeof(double));
    for (int i = 1; i < n_pts; i++) {
        filter_preds[i] = filter_preds[0] + i * n_neurons;
    }

    // Allocate array for times
    double* rt_times_us = (double *) malloc(n_pts * sizeof(double));

    for (int k = 0; k < n_pts; k++) {

        // Start clock
        struct timeval st, et;
        gettimeofday(&st, NULL);

        // Send spike counts
        if (send(sock, spks[k], n_neurons * sizeof(int), 0) < 0) {
            perror("send failed");
            return 1;
        }
  
        // Receive predictions from the server
        if (recv(sock, filter_preds[k], n_neurons * sizeof(double), 0) < 0) {
            perror("recv failed");
            return 1;
        }

        // Stop clock
        gettimeofday(&et, NULL);

        // Compute time (microseconds)
        rt_times_us[k] = (et.tv_sec - st.tv_sec) * 1e6 + (et.tv_usec - st.tv_usec);
    }

    // Compute mean latency
    double acc, rt_mean;
    for (int i = 0; i < n_pts; i++) {
        acc = acc + rt_times_us[i];
    }
    rt_mean = acc / n_pts;
    printf("Mean round-trip time: %f us\n", rt_mean);
  
    // Save output data
    printf("Writing data to %s\n", OUT_FPATH);
    save_data(OUT_FPATH, filter_preds[0], rt_times_us, n_pts, n_neurons);
    printf("Done.\n");

    // Free allocated memory
    free(rt_times_us);
    free(filter_preds[0]);
    free(filter_preds);
	free(spks[0]);
    free(spks);

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