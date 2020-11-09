//! Module that implements "filter mode"

use std::net::{IpAddr, TcpListener};

use crate::protocol::FilterConnection;


/// Run filter mode
pub fn run(host: IpAddr, port: u16) {

    // Run server and connect to probe mode
    let listener = TcpListener::bind((host, port)).unwrap();
    let (stream, _) = listener.accept().unwrap();
    let mut conn = FilterConnection::new(stream).unwrap();

    // Echo spikes back to probe
    while match conn.recv() {
        Ok(spks) => {
            conn.send(spks).unwrap();
            true
        },
        Err(_) => {
            println!("Connection error");
            false
        }
    } {}

    // Close socket server
    drop(listener);
}
