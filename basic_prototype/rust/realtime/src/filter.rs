//! Module that implements "filter mode"

use std::thread;
use std::net::{IpAddr, TcpListener, TcpStream, Shutdown};
use std::io::{Read, Write};


fn handle_client(mut stream: TcpStream) {

    // Read header
    let mut hdr_bytes = [0; 2];
    stream.read(&mut hdr_bytes).unwrap();
    let num_neurons = u16::from_be_bytes(hdr_bytes);

    // Write ACK
    let ack_buf = [0x06];
    stream.write(&ack_buf).unwrap();

    // Read and echo data
    let mut data = vec![0 as u8; num_neurons as usize];
    while match stream.read(&mut data) {
        Ok(size) => {
            // echo everything!
            stream.write(&data[0..size]).unwrap();
            true
        },
        Err(_) => {
            println!("An error occurred, terminating connection with {}", stream.peer_addr().unwrap());
            stream.shutdown(Shutdown::Both).unwrap();
            false
        }
    } {}
}

pub fn run(host: IpAddr, port: u16) {

    let listener = TcpListener::bind((host, port)).unwrap();
    println!("Server listening on port {}", port);

    // accept connections and process them, spawning a new thread for each one
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("New connection: {}", stream.peer_addr().unwrap());
                thread::spawn(move|| {
                    handle_client(stream)
                });
            }
            Err(e) => {
                println!("Error: {}", e);
            }
        }
    }

    // close the socket server
    drop(listener);
}
