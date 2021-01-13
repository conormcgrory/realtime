//! Filters

use ndarray::prelude::*;
use ndarray::stack;



/// Dummy 'filter' that returns unmodified input as 'prediction'
pub struct FilterAutoEcho {}

/// Trait for autoregressive adaptive filters
pub trait FilterAuto {
    fn predict_next(&mut self, x: &Array1<f64>) -> Array1<f64>;
}

impl FilterAuto for FilterAutoEcho {

    fn predict_next(&mut self, x: &Array1<f64>) -> Array1<f64> {
        return x.clone()
    }
}


/// Least-mean-squares AdaptiveFilter (needed for FilterAutoLMS)
pub struct FilterLMS {
    mu: f64,
    wts: Array2<f64>,
}

impl FilterLMS {

    pub fn new(dim: u16, order: u16, mu:f64) -> FilterLMS {

        let wts_shape = (dim as usize, (order * dim) as usize);

        FilterLMS {
            mu: mu,
            wts: Array::zeros(wts_shape),
        }
    }

    /// Adapt filter to new (desired, input) pair
    pub fn adapt(&mut self, d: &Array2<f64>, x: &Array2<f64>) {

        // Flatten input matrix into vector
        let x_vec = x.t().into_shape((self.wts.ncols(), 1)).unwrap();

        // Compute weight update
        let y: Array2<f64> = self.wts.dot(&x_vec);
        let e: Array2<f64> = d - &y;
        let wts_update = self.mu * e.dot(&x_vec.t());

        self.wts += &wts_update;
    }

    /// Predict value for input
    pub fn predict(&self, x: &Array2<f64>) -> Array2<f64> {

        let x_vec = x.t().into_shape((self.wts.ncols(), 1)).unwrap();
        self.wts.dot(&x_vec)
    }
}
    

/// FilterAuto that uses LMS to filter autoregressive process
pub struct FilterAutoLMS {
    flt_lms: FilterLMS,
    x_hist: Array2<f64>,
}

impl FilterAutoLMS {

    pub fn new(dim: u16, order: u16, mu: f64) -> FilterAutoLMS {
        FilterAutoLMS{
            flt_lms: FilterLMS::new(dim, order, mu),
            x_hist: Array::zeros((dim as usize, order as usize)),
        }
    }
}

impl FilterAuto for FilterAutoLMS {

    fn predict_next(&mut self, x: &Array1<f64>) -> Array1<f64> {

        // Convert x to 2D array
        let x_2d = x.clone().insert_axis(Axis(1));
        
        // Update filter using history as input and current value as output
        self.flt_lms.adapt(&x_2d, &self.x_hist);

        // Add current value to history, dropping oldest value 
        let x_hist_trunc = self.x_hist.slice(s![.., 0..-1]);
        self.x_hist = stack![Axis(1), x_2d, x_hist_trunc];

        // Use new history to predict next value
        let y_2d = self.flt_lms.predict(&self.x_hist);
        let y = y_2d.index_axis_move(Axis(1), 0);

        return y;
    }
}
