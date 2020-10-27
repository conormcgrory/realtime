"""Server that sends signal to client process"""

import socket
from time import time_ns

import h5py
import numpy as np


SIGNAL_FPATH = '/Users/cmcgrory/park_lab/realtime/basic_prototype/data/processed/r11_avg_fr.h5'
OUTPUT_FPATH = 'output.h5'

SERVER_HOST = 'localhost'
SERVER_PORT = 2000


def main():
    
    # Load signal from file
    with h5py.File(SIGNAL_FPATH, 'r') as f:
        signal = f['fr_avg_hz'][:]
        n_pts = signal.shape[0]

    # Create server and connect to client
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((SERVER_HOST, SERVER_PORT))
    sock.listen(1)
    conn, _ = sock.accept()

    # Output values from filter
    filter_preds = np.full(n_pts, np.nan)

    # Round-trip latencies (ms)
    rt_times = np.full(n_pts, np.nan)

    for i in range(n_pts):
       
        t_start_ns = time_ns()

        # Write next signal value to client
        conn.send(signal[i].tobytes())

        # Read filter value from client
        filter_preds[i] = np.frombuffer(conn.recv(8))[0]

        rt_times[i] = (time_ns() - t_start_ns) / 1000

    # Close connection
    sock.close()

    # Summary statistics
    mean_time = np.mean(rt_times)
    median_time = np.median(rt_times)
    print(f'Mean round-trip latency: {mean_time:.2f}')
    print(f'Median round-trip latency: {median_time:.2f}')

    # Save filter predictions and latencies
    with h5py.File(OUTPUT_FPATH, 'w') as f:
        f.create_dataset('filter_preds', data=filter_preds)
        f.create_dataset('rt_times_ms', data=rt_times)


if __name__ == '__main__':
    main()
