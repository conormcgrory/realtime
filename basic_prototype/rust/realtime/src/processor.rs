//! Module that implements "processor mode"

use std::thread;
use std::net::{IpAddr, TcpListener, TcpStream, Shutdown};
use std::io::{Read, Write};
use ndarray::prelude::*;
use crate::filters::EchoFilter;


fn handle_client(mut stream: TcpStream) {

    println!("Connected to probe at {}", stream.peer_addr().unwrap());

    // Read header
    println!("Receiving header information...");
    let mut hdr_bytes = [0; 2];
    stream.read(&mut hdr_bytes).unwrap();
    let num_neurons = u16::from_be_bytes(hdr_bytes);

    // Write ACK
    let ack_buf = [0x06];
    stream.write(&ack_buf).unwrap();
    println!("Done.");

    
    // TODO: Add this!
    // Convert spikes to 64-bit float array
    //let spks_f64: Vec<f64> = spks_buf.iter().map(|&x| x as f64).collect();
    //let spks_arr: Array1<f64> = Array::from(spks_buf);
 
     
    println!("Filtering signal...");
    let mut flt = EchoFilter{};
    
    loop {
       
        // Read next spike vector
        let mut spks_buf = vec![0 as u8; num_neurons as usize];
        if stream.read_exact(&mut spks_buf).is_err() {
            break;
        }

        // Run filter on spike vector
        let spks_arr = Array::from(spks_buf);
        let fpred = flt.predict_next(&spks_arr);

        // Write filter prediction to probe
        stream.write(&fpred.to_vec()).unwrap();
    }

    println!("Done.");
}

pub fn run(host: IpAddr, port: u16) {

    println!("Starting server at {}:{}...", host, port);
    let listener = TcpListener::bind((host, port)).unwrap();

    // Accept connections and process them, spawning a new thread for each one
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                thread::spawn(move|| {
                    handle_client(stream)
                });
            }
            Err(e) => {
                println!("Error: {}", e);
            }
        }
    }

    // Close the socket server
    drop(listener);
}
