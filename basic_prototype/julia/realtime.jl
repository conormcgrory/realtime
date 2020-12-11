# Real-time neural data filtering

using ArgParse
using Sockets
using HDF5
using Statistics
using Printf

const ACK_BYTE = 0x06
const N_PTS = 1000
const N_NEURONS = 698


function hdr_encode(n_neurons::Integer)
    return reinterpret(UInt8, UInt16[convert(UInt16, n_neurons)])
end


function hdr_decode(hdr_bytes::Array{UInt8})
    return reinterpret(UInt16, hdr_bytes)[1]
end


function spks_encode(spks::Array{Int8})
    return reinterpret(UInt8, spks)
end


function spks_decode(spk_bytes::Array{UInt8})
    return reinterpret(Int8, spk_bytes)
end


function fpred_encode(fpred::Array{Float64})
    return reshape(reinterpret(UInt8, fpred), :, 1)
end


function fpred_decode(fpred_bytes::Array{UInt8})
    return reinterpret(Float64, reshape(fpred_bytes, 8, :))
end


function probe_mode(host::IPAddr, port::Integer, in_fpath::String, out_fpath::String)

    # Load test signal from file (need to swap axes; signal stored by NumPy)
    println("Loading data from '$in_fpath'...")
    spks = h5open(in_fpath, "r") do file
        permutedims(read(file, "spks"), [2, 1])
    end
    println("Done.")
    
    # Number of neurons (must be less than 65535)
    n_neurons = size(spks, 1)

    println("Connecting to $host:$port...")
    conn = connect(host, port)
    println("Done.")

    println("Sending header information...")

    # Send header
    hdr_bytes = hdr_encode(n_neurons)
    write(conn, hdr_bytes)
    
    # Receive ACK byte
    hdr_resp = read(conn, UInt8)
    if hdr_resp != ACK_BYTE
        error("Response to header not ACK")
    end

    println("Done.")

    println("Sending signal...")

    filter_preds = fill(NaN, n_neurons, N_PTS)
    rt_times_us = fill(NaN, N_PTS)

    for i in 1:N_PTS

        t_start_ns = time_ns()

        # Write spike counts (just neuron 1)
        write(conn, spks_encode(spks[:, i]))

        # Read filter value from client (8 bytes per neuron)
        filter_preds[:, i] = fpred_decode(read(conn, n_neurons * 8))

        # Compute round-trip latency (microseconds)
        rt_times_us[i] = (time_ns() - t_start_ns) / 1000

    end
    println("Done.")

    println("Closing...")
    close(conn)
    println("Done.")

    # Print average round-trip time
    mean_time = mean(rt_times_us)
    median_time = median(rt_times_us)
    @printf("Mean round-trip latency: %.2f μs\n", mean_time)
    @printf("Median round-trip latency: %.2f μs\n", median_time)

    # Write filter predictions and round-trip times to output file
    println("Writing data to '$out_fpath'...")
    h5open(out_fpath, "w") do file
        write(file, "filter_preds", filter_preds)
        write(file, "rt_times_us", rt_times_us)
    end
    println("Done")

end


function processor_mode(host::IPAddr, port::Integer)

    # Connect to client
    println("Starting server at $host:$port...")
    server = listen(host, port)
    conn = accept(server)

    # TODO: Add address to printout
    println("Connected to probe.")

    println("Receiving header information...")

    # Receive header
    hdr_bytes = read(conn, 2)
    n_neurons = hdr_decode(hdr_bytes)
    
    # Send ACK
    write(conn, ACK_BYTE)

    println("Done.")

    println("Filtering signal...")

    while !eof(conn)

        # Receive spikes from probe (1 byte per neuron)
        spks = spks_decode(read(conn, n_neurons))

        # "Echo filter"
        fpred = convert(Array{Float64}, spks)

        # Send filter prediction to probe
        write(conn, fpred_encode(fpred))

    end

    println("Done.")

    close(conn)

end


function main()

    settings = ArgParseSettings()

    @add_arg_table settings begin
        "probe"
            help = "probe mode"
            action = :command
        "processor"
            help = "processor mode"
            action = :command
    end

    @add_arg_table settings["probe"] begin
        "--host", "-a"
            arg_type = IPv4 
        "--port", "-p"
            arg_type = Int
        "--input", "-i"
            arg_type = String
        "--output", "-o"
            arg_type = String
    end

    @add_arg_table settings["processor"] begin
        "--host", "-a"
            arg_type = IPv4 
        "--port", "-p"
            arg_type = Int
    end

    args = parse_args(settings)

    # TODO: Figure out how to parse IP address in ArgParse
    if args["%COMMAND%"] == "probe"
        cargs = args["probe"]
        probe_mode(cargs["host"], cargs["port"], cargs["input"], cargs["output"])
    elseif args["%COMMAND%"] == "processor"
        cargs = args["processor"]
        processor_mode(cargs["host"], cargs["port"])
    end
end

main()
