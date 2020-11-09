//! Data types used for transferring data between probe and filter

use std::net::TcpStream;
use std::io;
use std::io::{Read, Write};



/// Byte sent for 'acknowledge' signal
const ACK_BYTE: u8 = 0x06;

/// Header sent from probe to filter
pub struct Header {
    num_neurons: u16,
}

impl Header {

    /// Byte size of header
    const NUM_BYTES: usize = 2;

    /// Encode Header object as byte array
    fn to_bytes(&self) -> [u8; Self::NUM_BYTES] {
        self.num_neurons.to_be_bytes()
    }

    /// Decode Header object from byte array
    fn from_bytes(bytes: [u8; Self::NUM_BYTES]) -> Header {
        Header {
            num_neurons: u16::from_be_bytes(bytes),
        }
    }
}


pub struct ProbeConnection {
    stream: TcpStream,
    num_neurons: u16,
    buf_recv: Vec<u8>,
}

impl ProbeConnection {

    fn send_header(hdr: Header, stream: &mut TcpStream) -> io::Result<()> {
        stream.write_all(&hdr.to_bytes())
    }

    fn wait_for_ack(stream: &mut TcpStream) -> io::Result<()> {

        // Read 1-byte response from filter
        let mut buf_resp = [0; 1];
        stream.read_exact(&mut buf_resp)?;

        // Check for ACK character
        if buf_resp == [ACK_BYTE] {
            Ok(())
        } else {
            return Err(io::Error::new(io::ErrorKind::Other, "ACK not recieved"))
        }
    }

    pub fn new(mut stream: TcpStream, num_neurons: u16) -> io::Result<ProbeConnection> {
        
        let hdr = Header {
            num_neurons: num_neurons,
        };
        Self::send_header(hdr, &mut stream)?;
        Self::wait_for_ack(&mut stream)?;

        Ok(ProbeConnection {
            stream: stream,
            num_neurons: num_neurons,
            buf_recv: vec![0 as u8; num_neurons as usize],
        })
    }

    pub fn send(&mut self, spks: Vec<u8>) -> io::Result<()> {
        self.stream.write_all(&spks)
    }

    pub fn recv(&mut self) -> io::Result<Vec<u8>> {
        self.stream.read_exact(&mut self.buf_recv)?;
        Ok(self.buf_recv.to_vec())
    }
}


pub struct FilterConnection {
    stream: TcpStream,
    num_neurons: u16,
    buf_recv: Vec<u8>,
}

impl FilterConnection {

    fn recv_header(stream: &mut TcpStream) -> io::Result<Header> {

        let mut hdr_bytes = [0 as u8; Header::NUM_BYTES];
        stream.read_exact(&mut hdr_bytes)?;

        Ok(Header::from_bytes(hdr_bytes))
    }

    fn send_ack(stream: &mut TcpStream) -> io::Result<()> {
        stream.write_all(&[ACK_BYTE])
    }

    pub fn new(mut stream: TcpStream) -> io::Result<FilterConnection> {
        
        let hdr = Self::recv_header(&mut stream)?;
        Self::send_ack(&mut stream)?;

        Ok(FilterConnection {
            stream: stream,
            num_neurons: hdr.num_neurons,
            buf_recv: vec![0 as u8; hdr.num_neurons as usize],
        })
    }

    pub fn send(&mut self, spks: Vec<u8>) -> io::Result<()> {
        self.stream.write_all(&spks)
    }

    pub fn recv(&mut self) -> io::Result<Vec<u8>> {
        self.stream.read_exact(&mut self.buf_recv)?;
        Ok(self.buf_recv.to_vec())
    }
}
