//! Main module for realtime filtering system
//!
//! Parses command-line arguments and launches application in either "filter"
//! or "probe" mode.

use std::net::IpAddr;
use clap::{App, Arg, SubCommand, value_t};

mod protocol;
mod filter;
mod probe;


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
            filter::run(host, port);
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
