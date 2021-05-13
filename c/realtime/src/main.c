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

#include "protocol.h"
#include "filters.h"


// TODO: Replace these with arguments
#define HOST "127.0.0.1"
#define PORT 8889
#define IN_FPATH "../../data/processed/test_spks_clang.h5"
#define OUT_FPATH "../../data/results/c_lms.h5"
#define FILTER_TYPE "lms"

// Order of filter
#define FILTER_ORDER 5

// Filter learning rate
#define FILTER_MU 0.01

// Number of points to send for test
#define N_PTS_SEND 10000


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


// Compute the mean of a vector of values
double compute_mean(double* vals, int nvals) {

    double acc;
    for (int i = 0; i < nvals; i++) {
        acc = acc + vals[i];
    }

    return acc / nvals;
}


// Probe mode
int probe_mode(char* host, int port, char* in_fpath, char* out_fpath) {

    printf("Loading data from '%s'...\n", in_fpath);

    // Read number of data points and number of neurons from input file
    int n_pts, n_neurons;
    get_data_dims(in_fpath, &n_pts, &n_neurons);

    // Load spike counts into memory
    int* spks = (int *) malloc(n_pts * n_neurons * sizeof(int));
    load_data(in_fpath, spks);

    printf("Done.\n");

    // Connect to processor
    printf("Connecting to processor at %s:%d...\n", host, port);
    struct ProbeConnection conn;
    if (probe_connect(host, port, n_neurons, &conn) != 0) {
        fprintf(stderr, "Probe connection failed\n");
        return 1;
    }
    printf("Done.\n");
    
    // Array for storing filter predictions
    double* filter_preds = (double *) malloc(N_PTS_SEND * n_neurons * sizeof(double));

    // Array for storing round-trip times (microseconds)
    double* rt_times_us = (double *) malloc(N_PTS_SEND * sizeof(double));

    printf("Sending signal...\n");
    for (int k = 0; k < N_PTS_SEND; k++) {

        // Pointers to spikes and filter predictions for this time step
        int* spks_k = spks + (k * n_neurons);
        double* filter_preds_k = filter_preds + (k * n_neurons);

        // Start clock
        struct timeval st, et;
        gettimeofday(&st, NULL);

        // Send spike counts to processor
        if (probe_send(&conn, spks_k) != 0) {
            fprintf(stderr, "probe_send() failed\n");
            return 1;
        }
  
        // Receive filter predictions from processor
        if (probe_recv(&conn, filter_preds_k) != 0) {
            fprintf(stderr, "probe_recv() failed\n");
            return 1;
        }

        // Stop clock
        gettimeofday(&et, NULL);

        // Compute time (microseconds)
        rt_times_us[k] = (et.tv_sec - st.tv_sec) * 1e6 + (et.tv_usec - st.tv_usec);
    }
    printf("Done.\n");

    // Compute mean latency
    double rt_mean = compute_mean(rt_times_us, N_PTS_SEND);
    printf("Mean round-trip latency: %f us\n", rt_mean);
  
    // Save output data
    printf("Writing data to '%s'...\n", out_fpath);
    save_data(out_fpath, filter_preds, rt_times_us, N_PTS_SEND, n_neurons);
    printf("Done.\n");

    // Free allocated memory
    free(rt_times_us);
    free(filter_preds);
    free(spks);

    // Close connection
    probe_disconnect(&conn);

    return 0;
}


// Processor mode
int processor_mode(char* host, int port, int use_lms) {

    // Connect to probe
    printf("Connecting to probe at %s:%d...\n", host, port);
    struct ProcessorConnection conn;
    if (processor_connect(host, port, &conn) != 0) {
        fprintf(stderr, "Processor connection failed\n");
        return 1;
    }
    printf("Done.\n");

    // Create filter objects for LMS and echo filters (we only use one)
    struct FilterAutoLMS flt_lms;
    FilterAutoLMS_new(&flt_lms, conn.n_neurons, FILTER_ORDER, FILTER_MU);
    struct FilterAutoEcho flt_echo;
    FilterAutoEcho_new(&flt_echo, conn.n_neurons);

    // Arrays for storing spikes as int and double
    int* spks_int = (int*) malloc(conn.n_neurons * sizeof(int));
    double* spks_double = (double*) malloc(conn.n_neurons * sizeof(double));

    printf("Filtering signal...\n");
    while(1) {

        // Receive spikes (int) from probe
        if (processor_recv(&conn, spks_int) != 0) {
            fprintf(stderr, "processor_recv() failed\n");
            return 1;
        }

        // If probe has disconnected, break out of loop and return
        if (!conn.is_connected) {
            break;
        }

        // Convert spikes to doubles
        for (int i = 0; i < conn.n_neurons; i++) {
            spks_double[i] = (double) spks_int[i];
        }

        // Update filter and send predictions back to probe
        if (use_lms) {
            FilterAutoLMS_predict_next(&flt_lms, spks_double);
            if (processor_send(&conn, flt_lms.x_pred) != 0) {
                fprintf(stderr, "processor_send() failed\n");
                return 1;
            }
        }
        else {
            FilterAutoEcho_predict_next(&flt_echo, spks_double);
            if (processor_send(&conn, flt_echo.x_pred) != 0) {
                fprintf(stderr, "processor_send() failed\n");
                return 1;
            }
        }
    }
    printf("Done.\n");

    // Free memory
    free(spks_int);
    free(spks_double);

    // Delete filter
    FilterAutoEcho_delete(&flt_echo);
    FilterAutoLMS_delete(&flt_lms);

    // Close connection
    processor_disconnect(&conn);
    
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

        // Probe mode
        if (strcmp(argv[1], "probe") == 0) {
            return probe_mode(HOST, PORT, IN_FPATH, OUT_FPATH);
        }

        // Processor mode
        else if (strcmp(argv[1], "processor") == 0) {

            // Parse filter type
            if (strcmp(FILTER_TYPE, "lms") == 0) {
                return processor_mode(HOST, PORT, 1);
            }
            else if (strcmp(FILTER_TYPE, "echo") == 0) {
                return processor_mode(HOST, PORT, 0);
            }
            else {
                fprintf(stderr, "filter type '%s' not supported\n", FILTER_TYPE);
                return 1;
            }
        }

        // Invalid mode
        else {
            print_usage();
            return 0;
        }
    }

    return 0;
}
