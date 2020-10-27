# Server 

using HDF5
using Sockets
using Statistics
using Printf


# Path to file containing test signal
const SIGNAL_FPATH = "/Users/cmcgrory/park_lab/realtime/basic_prototype/data/processed/r11_avg_fr.h5"

# Path to output file
const OUTPUT_FPATH = "output.h5"

# IP and port to run server on
const SERVER_IP = ip"127.0.0.1"
const SERVER_PORT = 2000


"""
    send_signal(signal, conn)

Send `signal` to client process over `conn`, and store and return client 
process response and round-trip latency.
"""
function send_signal(signal::Array{Float64, 1}, conn::TCPSocket)

    n_pts = length(signal)

    # Output values from filter
    filter_preds = fill(NaN, n_pts)

    # Round-trip latencies (ms)
    rt_times = fill(NaN, n_pts)

    for i in 1:n_pts

        t_start_ns = time_ns()

        # Write next signal value to client
        write(conn, signal[i])

        # Read filter value from client
        filter_preds[i] = read(conn, Float64)

        rt_times[i] = (time_ns() - t_start_ns) / 1000

    end

    return filter_preds, rt_times

end


function main()

    # Load test signal from file
    fr_avg = h5read(SIGNAL_FPATH, "fr_avg_hz")
    n_pts = length(fr_avg)

    # Connect to client
    println("Server online. Waiting for client...")
    server = listen(SERVER_IP, SERVER_PORT)
    conn = accept(server)
    println("Client connected. Starting stream...")

    # Send signal 
    filter_preds, rt_times = send_signal(fr_avg, conn)

    # Close connection
    println("Stream complete. Closing...")
    close(conn)

    # Print average round-trip time
    mean_time = mean(rt_times)
    median_time = median(rt_times)
    @printf("Mean round-trip latency: %.2f ms\n", mean_time)
    @printf("Median round-trip latency: %.2f ms\n", median_time)

    # Write filter predictions and round-trip times to output file
    h5write(OUTPUT_FPATH, "filter_preds", filter_preds)
    h5write(OUTPUT_FPATH, "rt_times_ms", rt_times)

end

main()
