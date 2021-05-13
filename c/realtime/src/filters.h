/* Header file for filters */


#ifndef _FILTERS_H
#define _FILTERS_H


/* Autoregressive least-mean-squares filter */
struct FilterAutoLMS {

    // Dimension of signal
    int dim;

    // Order of filter (number of signal vectors in history)
    int order;
    
    // Size of history (dim * order)
    int hist_size;

    // Step size used for filter updates
    double mu;

    // Filter prediction (should always be equal to wts * x_hist)
    double* x_pred;

    // Filter error from last step
    double* x_err;

    // Signal history (represented as vector of concatenated signal vectors)
    double* x_hist;

    // Weight matrix (row-major)
    double* wts;
};

// Constructor for filter object
void FilterAutoLMS_new(struct FilterAutoLMS* flt, int dim, int order, double mu);

// Destructor for filter object
void FilterAutoLMS_delete(struct FilterAutoLMS* flt);

// Update filter with new signal value and predict next value
void FilterAutoLMS_predict_next(struct FilterAutoLMS* flt, double* x);


/* 'Echo' filter */
struct FilterAutoEcho {

    // Dimension of signal
    int dim;

    // Filter prediction
    double* x_pred;
};

// Constructor for filter object
void FilterAutoEcho_new(struct FilterAutoEcho* flt, int dim);

// Destructor for filter object
void FilterAutoEcho_delete(struct FilterAutoEcho* flt);

// Update filter with new signal value and predict next value
void FilterAutoEcho_predict_next(struct FilterAutoEcho* flt, double* x);


#endif
