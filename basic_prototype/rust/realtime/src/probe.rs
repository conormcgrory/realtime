//! Module that implements "probe mode"


use std::net::{IpAddr, TcpStream};
use std::io;
use std::io::{Read, Write};
use std::time::Instant;
use ndarray::prelude::*;
use hdf5;


/// Byte sent for 'acknowledge' signal
const ACK_BYTE: u8 = 0x06;


/// Construct f64 vector from byte vector
fn f64_vec_from_bytes(vec_1: &Vec<u8>) -> Vec<f64>{

    let n_floats = vec_1.len() / 8;
    let mut vec_2 = vec![0 as f64; n_floats];

    for i in 0..n_floats {

        // Load bytes for int into buffer
        let mut buf = [0; 8];
        for j in 0..8 {
            buf[j] = vec_1[i * 8 + j];
        }

        // Decode f64 from bytes
        vec_2[i] = f64::from_be_bytes(buf);
    }

    return vec_2;
}

/// Send header containing number of neurons in recording to processor
fn send_header(num_neurons: u16, stream: &mut TcpStream) -> io::Result<()> {

    // Encode header data and write header to socket
    let hdr_bytes = num_neurons.to_be_bytes();
    stream.write(&hdr_bytes).unwrap();

    // Read response from filter
    let mut resp_bytes = [0; 1];
    stream.read_exact(&mut resp_bytes).unwrap();

    // Check for ACK character
    if resp_bytes == [ACK_BYTE] {
        return Ok(());
    } else {
        return Err(io::Error::new(io::ErrorKind::Other, "ACK not recieved"));
    }
}

// Send spike vector to processor
fn send_spikes(stream: &mut TcpStream, spks: &Vec<u8>) -> io::Result<()> {
    stream.write_all(&spks)
}

// Recieve filter prediction from probe
fn recv_fpred(stream: &mut TcpStream, num_neurons: usize) -> io::Result<Vec<f64>> {

    // Read bytes from stream
    let mut buf = vec![0 as u8; num_neurons * 8];
    stream.read_exact(&mut buf)?;

    // Decode filter prediction from bytes
    let fpred = f64_vec_from_bytes(&buf);

    return Ok(fpred);
}


/// Send spike count vectors to filter and record and time response
fn send_signal(spks: Array2<u8>, stream: &mut TcpStream) -> io::Result<(Array2<f64>, Array1<f64>)> {

    let num_neurons = spks.len_of(Axis(0));
    let num_pts = spks.len_of(Axis(1));

    let mut filter_preds = Array::zeros((num_neurons, num_pts));
    let mut times_us = Array::zeros(num_pts);

    for i in 0..num_pts {

        // Start clock
        let t_start = Instant::now();

        // Write spike counts to socket
        let spk_vec = spks.column(i).to_vec();
        send_spikes(stream, &spk_vec).unwrap();

        // Read response from filter
        let fpred_vec = recv_fpred(stream, num_neurons).unwrap();
        let fpred = Array::from(fpred_vec);

        // Stop clock
        let time_ns = t_start.elapsed().as_nanos();
        times_us[i] = time_ns as f64 / 1000.0;

        // Add response to filter predictions
        filter_preds.column_mut(i).assign(&fpred);
    }

    return Ok((filter_preds, times_us));
}


// Run probe mode
pub fn run(host: IpAddr, port: u16, in_fpath: &str, out_fpath: &str) {

    // Read input file (TODO: Move this into its own function)
    println!("Loading data from {}...", in_fpath);
    let file = hdf5::File::open(in_fpath).unwrap();
    let dset = file.dataset("spks").unwrap();
    let spks = dset.read_2d::<u8>().unwrap();
    println!("Done.");

    println!("Connecting to {}:{}...", host, port);
    match TcpStream::connect((host, port)) {
        Ok(mut stream) => {
            println!("Done.");
            println!("Sending header information...");
            let num_neurons = spks.len_of(Axis(0));
            let num_pts = spks.len_of(Axis(1));
            send_header(num_neurons as u16, &mut stream).expect("Failed to send header");
            println!("Done.");

            println!("Sending signal...");
            let (filter_preds, times_us) = send_signal(spks, &mut stream).expect("Failed to send signal");
            println!("Done.");

            // Write results to output file (TODO: Move this into its own function)
            println!("Writing data to {}...", out_fpath);
            let file = hdf5::File::create(out_fpath).unwrap();
            let ds_filter_preds = file
                .new_dataset::<u8>()
                .create("filter_preds", (num_neurons, num_pts))
                .unwrap();
            ds_filter_preds.write(&filter_preds).unwrap(); 
            let ds_times_us = file
                .new_dataset::<f64>()
                .create("rt_times_us", num_pts)
                .unwrap();
            ds_times_us.write(&times_us).unwrap(); 
            println!("Done.");
        },
        Err(e) => {
            println!("Failed to connect: {}", e);
        }
    }
    println!("Terminated.");
}


