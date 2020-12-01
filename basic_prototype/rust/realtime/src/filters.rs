//! Filters

use ndarray::prelude::*;

// TODO: Add this once I understand traits
//pub trait AutoFilter {
//    fn predict_next(&self, x: &Vec<u8>) -> Vec<u8>;
//}

pub struct EchoFilter {}

impl EchoFilter {

    pub fn predict_next(&self, x: &Array1<f64>) -> Array1<f64> {
        return x.clone()
    }
}
