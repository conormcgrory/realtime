//! Filters

use ndarray::prelude::*;

// TODO: Add this once I understand traits
//pub trait AutoFilter {
//    fn predict_next(&self, x: &Vec<u8>) -> Vec<u8>;
//}

pub struct EchoFilter {}

impl EchoFilter {

    // TODO: Change this to f64
    pub fn predict_next(&self, x: &Array1<u8>) -> Array1<u8> {
        return x.clone()
    }
}
