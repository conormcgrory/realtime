#!/usr/bin/env python3
"""Script for creating test signal from Steinmetz data."""

import os

import h5py
import numpy as np
from matplotlib import pyplot as plt


# Directory where raw Steinmetz files are stored
RAW_DATA_DIR = 'data/raw'

# Filenames to load data from (in order!)
RAW_DATA_FNAMES = ['steinmetz_part0.npz', 'steinmetz_part1.npz', 'steinmetz_part2.npz']

# Output directory
PROCESSED_DATA_DIR = 'data/processed'

# Session to use for test signal
SESSION_NUM = 11


def main():

    # Load raw Steinmetz data from files
    print(f'Loading Steinmetz data from {RAW_DATA_DIR}...')
    input_fpaths = [os.path.join(RAW_DATA_DIR, f) for f in RAW_DATA_FNAMES]
    all_sessions = np.array([])
    for f in input_fpaths:
        all_sessions = np.hstack((all_sessions, np.load(f, allow_pickle=True)['dat']))
    print('Done.')
    
    # Select recording 
    data = all_sessions[SESSION_NUM]
    bin_size_sec = data['bin_size']
    spks = data['spks']
    n_neurons = spks.shape[0]
    n_trials = spks.shape[1]
    n_smps_trial = spks.shape[2]
    n_smps_total = n_trials * n_smps_trial
    print(f'Session: {SESSION_NUM}')
    print(f'Bin size: {bin_size_sec} sec')
    print(f'Num. trials: {n_trials}')
    print(f'Num. samples/trial: {n_smps_trial}')
    print(f'Num. samples total: {n_smps_total}')

    # Create test signal by concatenating data from all trials
    spks_all_trials = spks.reshape(n_neurons, -1)

    # Save test signal to HDF5 file
    out_fname = 'test_spks.h5'
    out_fpath = os.path.join(PROCESSED_DATA_DIR, out_fname)
    print(f'Writing to {out_fpath}...')
    with h5py.File(out_fpath, 'w') as f:
        dset = f.create_dataset('spks', data=spks_all_trials)
        dset.attrs['session_num'] = SESSION_NUM
        dset.attrs['bin_size_sec'] = bin_size_sec
    print('Done.')
    
    
if __name__ == '__main__':
    main()