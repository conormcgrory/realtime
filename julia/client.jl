# Client (reads signal from processor)

using Sockets


# IP and port number server is running on
const SERVER_IP = ip"127.0.0.1"
const SERVER_PORT = 2000


"""
    echo_stream(conn:TCPSocket)

Read 8-bit ints from `conn` and immediately write them back.
"""
function echo_stream(conn::TCPSocket)

    while !eof(conn)

        x = read(conn, Int8)
        write(conn, x)

    end
end


function main()

    # Connect to server
    println("Starting client...")
    conn = connect(SERVER_IP, SERVER_PORT)
    println("Connected to server. Processing signal...")

    # Run simple "echo filter" on signal
    echo_stream(conn)

    println("Stream finished. Closing...")

end

main()
