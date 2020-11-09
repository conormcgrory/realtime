//! Module that implements "probe mode"

use std::net::{IpAddr, TcpStream};
use std::time::Instant;

use ndarray::prelude::*;
use hdf5;

use crate::protocol::ProbeConnection;


/// Read spike data from input file
fn read_spks_file(fpath: &str) -> hdf5::Result<Array2<u8>> {

    hdf5::File::open(fpath)?
        .dataset("spks")?
        .read_2d::<u8>()
}

/// Write filter predictions and latency times to output file
fn write_output(filter_preds: Array2<u8>, times_us: Array1<f64>, out_fpath: &str) -> hdf5::Result<()> {

    let output_file = hdf5::File::create(out_fpath)?;

    // Write filter predictions
    let fp_shape = (filter_preds.len_of(Axis(0)), filter_preds.len_of(Axis(1)));
    output_file.new_dataset::<u8>()
        .create("filter_preds", fp_shape)?
        .write(&filter_preds)?; 

    // Write latency times
    let ts_shape = times_us.len_of(Axis(1));
    output_file.new_dataset::<f64>()
        .create("rt_times_us", ts_shape)?
        .write(&times_us)
}

/// Run probe mode
pub fn run(host: IpAddr, port: u16, in_fpath: &str, out_fpath: &str) -> () {

    // Read spikes from input file
    let spks = read_spks_file(in_fpath).expect("Unable to read input file");
    let num_neurons = spks.len_of(Axis(0));
    let num_pts = spks.len_of(Axis(1));

    // Connect to server
    let stream = TcpStream::connect((host, port)).unwrap();
    let mut conn = ProbeConnection::new(stream, num_neurons as u16).unwrap();
    println!("Successfully connected to server in port {}", port);

    // Send signal and record latencies
    let mut filter_preds = Array::zeros((num_neurons, num_pts));
    let mut times_us = Array::zeros(num_pts);

    for i in 0..num_pts {

        // Start clock
        let t_start = Instant::now();

        // Write spike counts to socket
        conn.send(spks.column(i).to_vec()).unwrap();

        // Read response from filter
        let fpred = Array::from(conn.recv().unwrap());

        // Stop clock
        let time_ns = t_start.elapsed().as_nanos();
        times_us[i] = time_ns as f64 / 1000.0;

        // Add response to filter predictions
        filter_preds.column_mut(i).assign(&fpred);
    }

    // Write output to file
    write_output(filter_preds, times_us, out_fpath).expect("Unable to write output");
}
