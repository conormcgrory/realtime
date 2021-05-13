/* Filters */

#include <stdlib.h>
#include <stdio.h>
#include <cblas.h>

#include "filters.h"


// Constructor for FilterAutoLMS object
void FilterAutoLMS_new(struct FilterAutoLMS* flt, int dim, int order, double mu) {

    // Number of elements in history
    int hist_size = dim * order;

    // Allocate array for prediction and set to zero
    flt->x_pred = (double *) malloc(dim * sizeof(double));
    for (int i = 0; i < dim; i++) {
        flt->x_pred[i] = 0.0;
    }

    // Allocate array for scaled error and set to zero
    flt->x_err = (double *) malloc(dim * sizeof(double));
    for (int i = 0; i < dim; i++) {
        flt->x_err[i] = 0.0;
    }

    // Allocate array for history and set to zero 
    flt->x_hist = (double *) malloc(hist_size * sizeof(double));
    for (int i = 0; i < hist_size; i++) {
        flt->x_hist[i] = 0.0;
    }

    // Allocate array for weights and set to zero
    flt->wts = (double *) malloc(dim * hist_size * sizeof(double));
    for (int i = 0; i < dim * hist_size; i++) {
        flt->wts[i] = 0.0;
    }

    // Populate fields
    flt->dim = dim;
    flt->order = order;
    flt->hist_size = hist_size;
    flt->mu = mu;
}


// Destructor for FilterAutoLMS object
void FilterAutoLMS_delete(struct FilterAutoLMS* flt) {

    // Free allocated memory
    free(flt->wts);
    free(flt->x_hist);
    free(flt->x_err);
    free(flt->x_pred);
}


// Update filter with new signal value and predict next value (using CBLAS)
void FilterAutoLMS_predict_next(struct FilterAutoLMS* flt, double* x) {

    // Compute error
    for (int i = 0; i < flt->dim; i++) {
        flt->x_err[i] = x[i] - flt->x_pred[i];
    }

    // Update weights (wts := wts + mu * x_err * x_hist')
    cblas_dger(
        CblasRowMajor, flt->dim, flt->hist_size, flt->mu, 
        flt->x_err, 1, flt->x_hist, 1, flt->wts, flt->hist_size
    );

    // Update history (x_hist[dim:end] := x_hist[0:end-dim]; x_hist[0:dim] := x)
    for (int i = (flt->hist_size - 1); i >= flt->dim; i--) {
        flt->x_hist[i] = flt->x_hist[i - flt->dim];
    }
    for (int i = 0; i < flt->dim; i++) {
        flt->x_hist[i] = x[i];
    }

    // Update prediction (x_pred := wts * x_hist)
    cblas_dgemv(
        CblasRowMajor, CblasNoTrans, flt->dim, flt->hist_size, 1.0, 
        flt->wts, flt->hist_size, flt->x_hist, 1, 0.0, flt->x_pred, 1
    );
}


// Constructor for FilterAutoEcho object
void FilterAutoEcho_new(struct FilterAutoEcho* flt, int dim) {

    // Allocate array for prediction and set to zero
    flt->x_pred = (double *) malloc(dim * sizeof(double));
    for (int i = 0; i < dim; i++) {
        flt->x_pred[i] = 0.0;
    }

    // Populate fields
    flt->dim = dim;
}


// Destructor for FilterAutoEcho object
void FilterAutoEcho_delete(struct FilterAutoEcho* flt) {

    // Free allocated memory
    free(flt->x_pred);
}


// Update filter with new signal value and predict next value
void FilterAutoEcho_predict_next(struct FilterAutoEcho* flt, double* x) {

    // Copy input to prediction
    for (int i = 0; i < flt->dim; i++) {
        flt->x_pred[i] = x[i];
    }
}
