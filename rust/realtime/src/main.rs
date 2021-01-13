//! Main module for realtime filtering system
//!
//! Parses command-line arguments and launches application in either "processor"
//! or "probe" mode.

use std::net::IpAddr;

use clap::{App, Arg, SubCommand, value_t};

mod processor;
mod probe;
mod filters;


fn main() {

    let matches = App::new("Real-time filtering")
        .version("0.0.1")
        .author("Conor McGrory <conor.mcgrory@stonybrook.edu>")
        .about("Real-time neural data filtering in Rust")
        .subcommand(
            SubCommand::with_name("processor")
                .about("Runs processor server")
                .arg(
                    Arg::with_name("host")
                        .long("host")
                        .short("a")
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
                )
                 .arg(
                    Arg::with_name("filter")
                        .long("filter")
                        .short("f")
                        .takes_value(true)
                        .required(true)
                        .help("Filter type ('lms' or 'echo')")
                ),
        )
        .subcommand(
            SubCommand::with_name("probe")
                .about("Runs probe client")
                .arg(
                    Arg::with_name("host")
                        .long("host")
                        .short("a")
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
                )
                .arg(
                    Arg::with_name("input")
                        .long("input")
                        .short("i")
                        .takes_value(true)
                        .required(true)
                        .help("Input file")
                )
                .arg(
                    Arg::with_name("output")
                        .long("output")
                        .short("o")
                        .takes_value(true)
                        .required(true)
                        .help("output file")
                ),
        )
        .get_matches();

    match matches.subcommand() {
        ("processor", Some(processor_matches)) => {
            let host = value_t!(processor_matches.value_of("host"), IpAddr)
                .unwrap_or_else(|e| e.exit());
            let port = value_t!(processor_matches.value_of("port"), u16)
                .unwrap_or_else(|e| e.exit());
            let filter = processor_matches.value_of("filter").unwrap();
            let use_lms = match filter {
                "echo" => false,
                "lms" => true,
                _ => panic!("Invalid filter!")
            };
            processor::run(host, port, use_lms);
        }
        ("probe", Some(probe_matches)) => {
            let host = value_t!(probe_matches.value_of("host"), IpAddr)
                .unwrap_or_else(|e| e.exit());
            let port = value_t!(probe_matches.value_of("port"), u16)
                .unwrap_or_else(|e| e.exit());
            let in_file = probe_matches.value_of("input").unwrap();
            let out_file = probe_matches.value_of("output").unwrap();
            probe::run(host, port, in_file, out_file);
        }
        ("", None) => println!("Use '--help' mode for program information."),
        _ => unreachable!(),
    }
}
