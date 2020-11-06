//! Module that implements "probe mode"


use std::net::{IpAddr, TcpStream};
use std::io;
use std::io::{Read, Write};
use std::time::Instant;
use ndarray::prelude::*;
use hdf5;


/// Send header to filter containing number of neurons in recording
fn send_header(num_neurons: u16, stream: &mut TcpStream) -> io::Result<()> {

    // Encode header data to byte array
    let hdr_bytes = num_neurons.to_be_bytes();

    // Write header to socket
    stream.write(&hdr_bytes).unwrap();

    // Read response from filter
    let mut resp_bytes = [0; 1];
    stream.read_exact(&mut resp_bytes).unwrap();

    // Check for ACK character
    if resp_bytes == [0x06] {
        return Ok(());
    } else {
        return Err(io::Error::new(io::ErrorKind::Other, "ACK not recieved"));
    }
}


/// Send spike count vectors to filter and record and time response
fn send_signal(spks: Array2<u8>, stream: &mut TcpStream) -> io::Result<(Array2<u8>, Array1<f64>)> {

    let num_neurons = spks.len_of(Axis(0));
    let num_pts = spks.len_of(Axis(1));

    let mut filter_preds = Array::zeros((num_neurons, num_pts));
    let mut times_us = Array::zeros(num_pts);

    for i in 0..num_pts {

        // Start clock
        let t_start = Instant::now();

        // Write spike counts to socket
        let buf_out = spks.column(i).to_vec();
        stream.write(&buf_out).unwrap();

        // Read response from filter
        let mut buf_in = vec![0 as u8; num_neurons];
        stream.read_exact(&mut buf_in).unwrap();
        let fpred = Array::from(buf_in);

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
    let file = hdf5::File::open(in_fpath).unwrap();
    let dset = file.dataset("spks").unwrap();
    let spks = dset.read_2d::<u8>().unwrap();

    match TcpStream::connect((host, port)) {
        Ok(mut stream) => {

            println!("Successfully connected to server in port {}", port);

            println!("Sending header...");
            let num_neurons = spks.len_of(Axis(0));
            let num_pts = spks.len_of(Axis(1));
            send_header(num_neurons as u16, &mut stream).expect("Failed to send header");
            println!("Done.");

            println!("Sending signal...");
            let (filter_preds, times_us) = send_signal(spks, &mut stream).expect("Failed to send signal");
            println!("Done.");

            // Write results to output file (TODO: Move this into its own function)
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
        },
        Err(e) => {
            println!("Failed to connect: {}", e);
        }
    }
    println!("Terminated.");
}


