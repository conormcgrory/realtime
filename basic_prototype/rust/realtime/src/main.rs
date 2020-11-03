use std::thread;
use std::net::{IpAddr, TcpListener, TcpStream, Shutdown};
use std::io::{Read, Write};

use clap::{App, Arg, SubCommand, value_t};
use hdf5;


static NUM_NEURONS: u16 = 698;


// Server helper
fn handle_client(mut stream: TcpStream) {

    let mut data = vec![0 as u8; NUM_NEURONS as usize];

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

// Server
fn filter_mode(host: IpAddr, port: u16) {

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


// Client 
fn probe_mode(host: IpAddr, port: u16, in_fpath: &str, out_fpath: &str) {

    // Read input file
    let file = hdf5::File::open(in_fpath).unwrap();
    let dset = file.dataset("spks").unwrap();
    let spks = dset.read_2d::<u8>().unwrap();

    match TcpStream::connect((host, port)) {
        Ok(mut stream) => {

            println!("Successfully connected to server in port {}", port);
            println!("Sending signal...");

            for s in spks.gencolumns() {

                let buffer = s.to_vec();
                stream.write(&buffer).unwrap();

                let mut data = vec![0 as u8; NUM_NEURONS as usize];
                match stream.read_exact(&mut data) {
                    Ok(_) => {
                        if &data != &buffer {
                            println!("Unexpected reply!");
                        } 
                    },
                    Err(e) => {
                        println!("Failed to receive data: {}", e);
                    }
                }
            }

            println!("Done.");
        },
        Err(e) => {
            println!("Failed to connect: {}", e);
        }
    }
    println!("Terminated.");
}


fn main() {

    let matches = App::new("Real-time filtering")
        .version("0.0.1")
        .author("Conor McGrory <conor.mcgrory@stonybrook.edu>")
        .about("Real-time neural data filtering in Rust")
        .subcommand(
            SubCommand::with_name("filter")
                .about("Runs filter server")
                .arg(
                    Arg::with_name("host")
                        .long("host")
                        .short("h")
                        .takes_value(true)
                        .required(true)
                        .help("Host IP")
                )
                .arg(
                    Arg::with_name("port")
                        .long("port")
                        .short("p")
                        .takes_value(true)
                        .required(true)
                        .help("Port")
                ),
        )
        .subcommand(
            SubCommand::with_name("probe")
                .about("Runs probe client")
                .arg(
                    Arg::with_name("host")
                        .short("h")
                        .takes_value(true)
                        .required(true)
                        .help("Host IP")
                )
                .arg(
                    Arg::with_name("port")
                        .short("p")
                        .takes_value(true)
                        .required(true)
                        .help("Port")
                )
                .arg(
                    Arg::with_name("input")
                        .short("i")
                        .takes_value(true)
                        .required(true)
                        .help("Input file")
                )
                .arg(
                    Arg::with_name("output")
                        .short("o")
                        .takes_value(true)
                        .required(true)
                        .help("output file")
                ),
        )
        .get_matches();

    match matches.subcommand() {
        ("filter", Some(filter_matches)) => {
            let host = value_t!(filter_matches.value_of("host"), IpAddr)
                .unwrap_or_else(|e| e.exit());
            let port = value_t!(filter_matches.value_of("port"), u16)
                .unwrap_or_else(|e| e.exit());
            filter_mode(host, port);
        }
        ("probe", Some(probe_matches)) => {
            let host = value_t!(probe_matches.value_of("host"), IpAddr)
                .unwrap_or_else(|e| e.exit());
            let port = value_t!(probe_matches.value_of("port"), u16)
                .unwrap_or_else(|e| e.exit());
            let in_file = probe_matches.value_of("input").unwrap();
            let out_file = probe_matches.value_of("output").unwrap();
            probe_mode(host, port, in_file, out_file);
        }
        ("", None) => println!("Use '--help' mode for program information."),
        _ => unreachable!(),
    }
}
