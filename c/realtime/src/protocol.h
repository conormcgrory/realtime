/* Header file for protocol */

#ifndef _PROTOCOL_H
#define _PROTOCOL_H


/* Connection interface for 'probe'
 *
 * The machine running in 'probe' mode connects to the machine running in
 * 'processor' mode by creating a ProbeConnection struct and calling 
 * probe_connect() to populate its members. This struct allows it to use the
 * probe_send() and probe_receive() functions, and can be closed using the
 * probe_disconnect() function.
 */

// Connection object for 'probe' mode
struct ProbeConnection {

    // IP address
    char* host;

    // Port
    int port;

    // Socket ID
    int sock_id;
    
    // Number of neurons whose data is being sent across the connection (this
    // is the length of the spike and filter prediction vectors)
    int n_neurons;

    // Connection status (0 for disconnected, 1 for connected)
    int is_connected;
};

// Connect to processor ('constructor' function for ProbeConnection)
int probe_connect(char* host, int port, int n_neurons, struct ProbeConnection* conn);

// Disconnect from processor ('destructor' function for ProbeConnection)
int probe_disconnect(struct ProbeConnection* conn);

// Send array of spikes to processor
int probe_send(struct ProbeConnection* conn, int* spks);

// Receive array of filter predictions from processor
int probe_recv(struct ProbeConnection* conn, double* fpreds);


/* Connection interface for 'processor'
 *
 * The machine running in 'processor' mode connects to the machine running in
 * 'probe' mode by creating a ProcessorConnection struct and calling 
 * processor_connect() to populate its members. This struct allows it to use the
 * processor_send() and processor_receive() functions, and can be closed using
 * the processor_disconnect() function.
 */

// Connection object for 'processor' mode
struct ProcessorConnection {

    // IP address
    char* host;

    // Port
    int port;

    // Socket ID used to listen for incoming connections
    int sock_desc_id;

    // Socket ID for connection with probe
    int sock_client_id;

    // Number of neurons whose data is being sent across the connection (this
    // is the length of the spike and filter prediction vectors)
    int n_neurons;

    // Connection status (0 for disconnected, 1 for connected)
    int is_connected;
};

// Connect to probe ('constructor' function for ProcessorConnection)
int processor_connect(char* host, int port, struct ProcessorConnection* conn);

// Disconnect from probe('destructor' function for ProcessorConnection)
int processor_disconnect(struct ProcessorConnection* conn);

// Send array of filter predictions to probe
int processor_send(struct ProcessorConnection* conn, double* fpreds);
    
// Receive array of spikes from probe
int processor_recv(struct ProcessorConnection* conn, int* spks);

#endif
