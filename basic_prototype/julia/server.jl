# Server 

using HDF5
using Sockets
using Statistics
using Printf


# Path to file containing test signal
#const SIGNAL_FPATH = "/Users/cmcgrory/park_lab/realtime/basic_prototype/data/processed/r11_spks.h5"

# Path to output file
#const OUTPUT_FPATH = "output.h5"

# IP and port to run server on
const SERVER_IP = ip"127.0.0.1"
const SERVER_PORT = 2000

# Number of neurons
#const NUM_NEURONS = 698


"""
    send_signal(signal, conn)

Send `signal` to client process over `conn`, and store and return client 
process response and round-trip latency (microseconds).
"""
function send_signal(signal::Array{Int8, 2}, conn::TCPSocket)

    n_neurons, n_pts = size(signal)

    # Output values from filter
    filter_preds = fill(NaN, n_pts)

    # Round-trip latencies (microseconds)
    rt_times = fill(NaN, n_pts)

    for i in 1:n_pts

        t_start_ns = time_ns()

        # Write signal value to client (only using neuron 1 right now)
        write(conn, signal[1, i])

        # Read filter value from client
        filter_preds[i] = read(conn, Int8)

        # Compute round-trip latency (microseconds)
        rt_times[i] = (time_ns() - t_start_ns) / 1000

    end

    return filter_preds, rt_times

end


function main()

    # Parse program arguments
    signal_fpath = ARGS[1]
    output_fpath = ARGS[2]

    # Load test signal from file (need to swap axes; signal stored by NumPy)
    println("Reading signal from '$signal_fpath'...")
    spks = h5open(signal_fpath, "r") do file
        permutedims(read(file, "spks"), [2, 1])
    end
    println("Done")

    # Connect to client
    println("Server online. Waiting for client...")
    server = listen(SERVER_IP, SERVER_PORT)
    conn = accept(server)
    println("Client connected. Starting stream...")

    # Send signal
    filter_preds, rt_times = send_signal(spks, conn)

    # Close connection
    println("Stream complete. Closing...")
    close(conn)

    # Print average round-trip time
    mean_time = mean(rt_times)
    median_time = median(rt_times)
    @printf("Mean round-trip latency: %.2f μs\n", mean_time)
    @printf("Median round-trip latency: %.2f μs\n", median_time)

    # Write filter predictions and round-trip times to output file
    println("Writing results to '$output_fpath'...")
    h5open(output_fpath, "w") do file
        write(file, "filter_preds", filter_preds)
        write(file, "rt_times_us", rt_times)
    end
    println("Done")

end

main()
