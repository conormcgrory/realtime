/* Autoregressive least-mean-squares filter */

#include <stdlib.h>
#include <stdio.h>

#include "filter_auto_lms.h"


void FilterAutoLMS_new(struct FilterAutoLMS* flt, int dim, int order, double mu) {

    // Number of elements in history
    int hist_size = dim * order;

    // Allocate array for prediction and set to zero
    flt->x_pred = (double *) malloc(dim * sizeof(double));
    for (int i = 0; i < dim; i++) {
        flt->x_pred[i] = 0.0;
    }

    // Allocate array for scaled error and set to zero
    flt->mu_err = (double *) malloc(dim * sizeof(double));
    for (int i = 0; i < dim; i++) {
        flt->mu_err[i] = 0.0;
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

void FilterAutoLMS_delete(struct FilterAutoLMS* flt) {

    // Free allocated memory
    free(flt->wts);
    free(flt->x_hist);
    free(flt->mu_err);
    free(flt->x_pred);
}

void FilterAutoLMS_predict_next(struct FilterAutoLMS* flt, double* x) {

    // Compute learning rate (mu) times error vector
    for (int i = 0; i < flt->dim; i++) {
        flt->mu_err[i] = flt->mu * (x[i] - flt->x_pred[i]);
    }

    // Update weights
    for (int i = 0; i < flt->dim; i++) {
        for (int j = 0; j < flt->hist_size; j++) {
            flt->wts[(i * flt->hist_size) + j] += flt->mu_err[i] * flt->x_hist[j];
        }
    }

    // Update history
    for (int i = (flt->hist_size - 1); i >= flt->dim; i--) {
        flt->x_hist[i] = flt->x_hist[i - flt->dim];
    }
    for (int i = 0; i < flt->dim; i++) {
        flt->x_hist[i] = x[i];
    }

    // Update prediction
    for (int i = 0; i < flt->dim; i++) {
        int idx = i * flt->hist_size;
        double acc = 0;
        for (int j = 0; j < flt->hist_size; j++) {
            acc += flt->wts[idx + j] * flt->x_hist[j];
        }
        flt->x_pred[i] = acc;
    }
}
