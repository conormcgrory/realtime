"""Real-time data analysis using Python"""

import argparse
import socket
from time import time_ns

import h5py
import numpy as np


NUM_NEURONS = 698


def probe_mode(host, port, in_fpath, out_fpath):

    # Load signal from file
    with h5py.File(in_fpath, 'r') as f:
        spks = f['spks'][:]
        assert(spks.shape[0] == NUM_NEURONS)
        n_pts = spks.shape[1]

    # Create socket and connect to server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))

    filter_preds = np.full((NUM_NEURONS, n_pts), np.nan)
    rt_times_us = np.full(n_pts, np.nan)

    for i in range(n_pts):
       
        t_start_ns = time_ns()

        # Write next signal value to client
        sock.send(spks[:, i].tobytes())

        # Read filter value from client
        buf_in = sock.recv(NUM_NEURONS)
        filter_preds[:, i] = np.frombuffer(buf_in, dtype=np.uint8)

        rt_times_us[i] = (time_ns() - t_start_ns) / 1000

    # Summary statistics
    mean_time = np.mean(rt_times_us)
    median_time = np.median(rt_times_us)
    print(f'Mean round-trip latency: {mean_time:.2f} us')
    print(f'Median round-trip latency: {median_time:.2f} us')

    # Save filter predictions and latencies
    with h5py.File(out_fpath, 'w') as f:
        f.create_dataset('filter_preds', data=filter_preds)
        f.create_dataset('rt_times_us', data=rt_times_us)


def processor_mode(host, port):

    # Create server and connect to client
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((host, port))
    sock.listen(1)
    conn, _ = sock.accept()

    while True:

        buf_in = conn.recv(NUM_NEURONS)
        if not buf_in: 
            break

        x = np.frombuffer(buf_in, dtype=np.uint8)
        conn.send(x.tobytes())

    # Close connection
    sock.close()


def parse_args():

    parser = argparse.ArgumentParser(
        description='Real-time filtering of neural data'
    )
    subparsers = parser.add_subparsers(dest='mode', required=True)

    # Probe subcommand
    probe_parser = subparsers.add_parser('probe', help='Probe mode')
    probe_parser.add_argument('-a', '--host', required=True)
    probe_parser.add_argument('-p', '--port', required=True)
    probe_parser.add_argument('-i', '--input', required=True)
    probe_parser.add_argument('-o', '--output', required=True)

    # Processor subcommand
    processor_parser = subparsers.add_parser('processor', help='Processor mode')
    processor_parser.add_argument('-a', '--host', required=True)
    processor_parser.add_argument('-p', '--port', required=True)
 
    return parser.parse_args()


def main():

    args = parse_args()
    port = int(args.port)
    
    if args.mode == 'probe':
        probe_mode(args.host, port, args.input, args.output)
    elif args.mode == 'processor':
        processor_mode(args.host, port)
    else:
        raise ValueError(f'"{args.mode}" not valid mode')


if __name__ == '__main__':
    main()
