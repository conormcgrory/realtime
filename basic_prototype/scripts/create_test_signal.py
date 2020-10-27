"""Script for creating test signal from Steinmetz data."""

import os

import h5py
import numpy as np
from matplotlib import pyplot as plt

# Directory where raw Steinmetz files are stored
INPUT_DIR = '/Users/cmcgrory/park_lab/realtime/basic_prototype/data/raw'

# Filenames to load data from (in order!)
INPUT_FNAMES = ['steinmetz_part0.npz', 'steinmetz_part1.npz', 'steinmetz_part2.npz']

# Output directory
OUTPUT_DIR = '/Users/cmcgrory/park_lab/realtime/basic_prototype/data/processed'

# Recording to use for test signal
REC_INDEX = 11


def main():

    # Load raw Steinmetz data from files
    print(f'Loading Steinmetz data from {INPUT_DIR}...')
    input_fpaths = [os.path.join(INPUT_DIR, f) for f in INPUT_FNAMES]
    all_recs = np.array([])
    for f in input_fpaths:
        all_recs = np.hstack((all_recs, np.load(f, allow_pickle=True)['dat']))
    print('Done.')
    
    # Select recording 
    data = all_recs[REC_INDEX]
    bin_size = data['bin_size']
    spks = data['spks']
    n_neurons = spks.shape[0]
    n_sessions = spks.shape[1]
    n_smps_session = spks.shape[2]
    n_smps_total = n_sessions * n_smps_session
    print(f'Recording: {REC_INDEX}')
    print(f'Bin size: {bin_size} sec')
    print(f'Num. sessions: {n_sessions}')
    print(f'Num. samples/session: {n_smps_session}')
    print(f'Num. samples total: {n_smps_total}')

    # Compute population firing rate for concatenated sessions
    print('Computing population average firing rate...')
    spks_all_sessions = data['spks'].reshape(n_neurons, -1)
    fr_avg = np.mean(spks_all_sessions, 0) / bin_size
    print('Done.')

    # Save test signal to HDF5 file
    out_fname = f'r{REC_INDEX:02d}_avg_fr.h5'
    out_fpath = os.path.join(OUTPUT_DIR, out_fname)
    print(f'Writing to {out_fpath}...')
    with h5py.File(out_fpath, 'w') as f:
        f.create_dataset('fr_avg_hz', data=fr_avg)
    print('Done.')
    
    
if __name__ == '__main__':
    main()
