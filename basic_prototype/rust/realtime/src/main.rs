use std::thread;
use std::net::{TcpListener, TcpStream, Shutdown};
use std::io::{Read, Write};
use std::str::from_utf8;

use clap::{App, Arg, SubCommand};

/*
// Server helper
fn handle_client(mut stream: TcpStream) {
    let mut data = [0 as u8; 50]; // using 50 byte buffer
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
fn filter() {
    let listener = TcpListener::bind("0.0.0.0:3333").unwrap();
    // accept connections and process them, spawning a new thread for each one
    println!("Server listening on port 3333");
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                println!("New connection: {}", stream.peer_addr().unwrap());
                thread::spawn(move|| {
                    // connection succeeded
                    handle_client(stream)
                });
            }
            Err(e) => {
                println!("Error: {}", e);
                /* connection failed */
            }
        }
    }
    // close the socket server
    drop(listener);
}


// Client 
fn client() {
    match TcpStream::connect("localhost:3333") {
        Ok(mut stream) => {
            println!("Successfully connected to server in port 3333");

            let msg = b"Hello!";

            stream.write(msg).unwrap();
            println!("Sent Hello, awaiting reply...");

            let mut data = [0 as u8; 6]; // using 6 byte buffer
            match stream.read_exact(&mut data) {
                Ok(_) => {
                    if &data == msg {
                        println!("Reply is ok!");
                    } else {
                        let text = from_utf8(&data).unwrap();
                        println!("Unexpected reply: {}", text);
                    }
                },
                Err(e) => {
                    println!("Failed to receive data: {}", e);
                }
            }
        },
        Err(e) => {
            println!("Failed to connect: {}", e);
        }
    }
    println!("Terminated.");
}
*/

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
            let host = filter_matches.value_of("host").unwrap();
            let port = filter_matches.value_of("port").unwrap();
            println!("filter");
            //filter(host, port);
        }
        ("probe", Some(probe_matches)) => {
            let host = probe_matches.value_of("host").unwrap();
            let port = probe_matches.value_of("port").unwrap();
            let in_file = probe_matches.value_of("input").unwrap();
            let out_file = probe_matches.value_of("output").unwrap();
            println!("probe");
            //probe(host, port, in_file, out_file);
        }
        ("", None) => println!("Use '--help' mode for program information."),
        _ => unreachable!(),
    }
}
