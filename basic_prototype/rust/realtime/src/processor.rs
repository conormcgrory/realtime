//! Module that implements "processor mode"

use std::thread;
use std::net::{IpAddr, TcpListener, TcpStream};
use std::io;
use std::io::{Read, Write};
use ndarray::prelude::*;
use crate::filters::FilterAutoEcho;


/// Byte sent for 'acknowledge' signal
const ACK_BYTE: u8 = 0x06;

// Number of past signal values to use for filter
const FILTER_ORDER: u16 = 5;

// Learning rate for filter
const FILTER_MU: f64 = 0.0001;


/// Encode f64 vector as byte vector
fn f64_vec_to_bytes(vec_1: &Vec<f64>) -> Vec<u8> {

    let n_floats = vec_1.len();
    let mut vec_2 = vec![0; n_floats * 8];

    for i in 0..n_floats {
        let buf = f64::to_be_bytes(vec_1[i]);
        for j in 0..8 {
            vec_2[i + j] = buf[j];
        }
    }

    return vec_2;
}

// Recieve header data from probe
fn recv_header(stream: &mut TcpStream) -> io::Result<u16> {

    // Read header bytes from stream
    let mut hdr_bytes = [0 as u8; 2];
    stream.read_exact(&mut hdr_bytes)?;

    // Send 'acknowledge' byte
    stream.write_all(&[ACK_BYTE])?;

    // Parse number of neurons from header and return
    Ok(u16::from_be_bytes(hdr_bytes))
}

// Send filter prediction to probe
fn send_fpred(stream: &mut TcpStream, spks: &Vec<f64>) -> io::Result<()> {

    // Encode spike vector
    let buf_out = f64_vec_to_bytes(spks);

    // Write bytes to stream
    stream.write_all(&buf_out)
}

// Recieve spikes from probe
fn recv_spikes(stream: &mut TcpStream, num_neurons: u16) -> io::Result<Vec<u8>> {

    let mut buf = vec![0 as u8; num_neurons as usize];
    stream.read_exact(&mut buf)?;
    Ok(buf.to_vec())
}

fn handle_client(mut stream: TcpStream) {

    println!("Connected to probe at {}", stream.peer_addr().unwrap());

    // Read header
    println!("Receiving header information...");
    let num_neurons = recv_header(&mut stream).unwrap();
    println!("Done.");

    println!("Filtering signal...");
    //let mut flt = FilterAutoLMS::new(num_neurons, FILTER_ORDER, FILTER_MU);
    let mut flt = FilterAutoEcho{};
    
    loop {
       
        // Read next spike vector
        let res = recv_spikes(&mut stream, num_neurons);
        if res.is_err() {
            break;
        }
        let spks_u8: Vec<u8> = res.unwrap();

        // Convert spike vector to f64 for filter
        let spks_f64: Vec<f64> = spks_u8
            .iter()
            .map(|&x| x as f64)
            .collect();
        let spks = Array::from(spks_f64);

        // Run filter on spike vector
        let fpred = flt.predict_next(&spks);

        // Write filter prediction to probe
        send_fpred(&mut stream, &fpred.to_vec()).unwrap();
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
