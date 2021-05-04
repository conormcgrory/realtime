/* Header file for protocol */

#ifndef _PROTOCOL_H
#define _PROTOCOL_H


struct ProbeConnection {
    char* host;
    int port;
    int sock_id;
    int n_neurons;
    int is_connected;
};

struct ProcessorConnection {
    char* host;
    int port;
    int sock_desc_id;
    int sock_client_id;
    int n_neurons;
    int is_connected;
};


int probe_connect(char* host, int port, int n_neurons, struct ProbeConnection* conn);

int probe_disconnect(struct ProbeConnection* conn);

int probe_send(struct ProbeConnection* conn, int* spks);

int probe_recv(struct ProbeConnection* conn, double* fpreds);


int processor_connect(char* host, int port, struct ProcessorConnection* conn);

int processor_disconnect(struct ProcessorConnection* conn);

int processor_send(struct ProcessorConnection* conn, double* fpreds);
    
int processor_recv(struct ProcessorConnection* conn, int* spks);

#endif
