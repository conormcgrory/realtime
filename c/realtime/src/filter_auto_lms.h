/* Header file for autoregressive least-mean-squares filter */

#ifndef _FILTER_AUTO_LMS_H
#define _FILTER_AUTO_LMS_H


struct FilterAutoLMS {

    int dim;
    int order;
    int hist_size;
    double mu;

    double* x_pred;
    double* mu_err;
    double* x_hist;
    double* wts;
};

void FilterAutoLMS_new(struct FilterAutoLMS* flt, int dim, int order, double mu);

void FilterAutoLMS_delete(struct FilterAutoLMS* flt);

void FilterAutoLMS_predict_next(struct FilterAutoLMS* flt, double* x);

#endif
