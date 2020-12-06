"""Real-time neural data filtering using Python.

The current iteration of this code is meant as a prototype to test if Python
is a suitable language to use for the production system. Because one large
concern with Python is speed, this program measures the round-trip latency
of each set of spike counts it sends, and saves the times to the output file
for further analysis.
"""

import argparse
import socket
from time import time_ns

import h5py
import numpy as np


# Number of bytes in header
HDR_SIZE = 2

# Byte used for "acknowledge" signal
ACK_BYTE = b'\x06'

# Number of past signal values to use for filter
FILTER_ORDER = 5

# Learning rate for filter
FILTER_MU = 0.01

# Number of points in signal to read
N_PTS = 10000


class FilterLMS:
    """This class represents an adaptive LMS filter.
    
    Args:
        dim (int): Dimension of signal
        order (int): Order of filter
        mu (float, optional): Learning rate. Also known as step size. If it is
            too slow, the filter may have bad performance. If it is too high,
            the filter will be unstable. The default value can be unstable for
            ill-conditioned input data.
        wts_init (str, optional): Method for initializing weights. Options are:
            - 'random': Create random weights
            - 'zeros': Create zero value weights
    """
    
    def __init__(self, dim, order, mu=0.01, wts_init='random'):
        
        self.dim = self._check_int_param(dim, 0, None)
        self.order = self._check_int_param(order, 0, None)
        self.mu = self._check_float_param(mu, 0, 1000)
        self.wts_shape = (self.dim, self.order * self.dim)
        
        if wts_init == 'random':
            self.wts = np.random.normal(0, 0.5, self.wts_shape)
        elif wts_init == 'zeros':
            self.wts = np.zeros(self.wts_shape)
        else:
            raise ValueError(f'"{wts_init}" not valid option for weight init')
        
    @staticmethod
    def _check_int_param(param, lo, hi):
        """Check if parameter is int and within given range.
        
        Args:
            param (convertible to int): Parameter to check
            lo (int): Lowest allowed value, or None
            hi (int): Highest allowed value, or None
            
        Returns:
            (int): Parameter converted to int
        """
        
        try:
            param = int(param)            
        except ValueError:
            raise ValueError('Parameter is not int or similar')
        else:
            if lo is None:
                lo = float('-inf')
            if hi is None:
                hi = float('inf')
            if not lo <= param <= hi:
                raise ValueError('Parameter is not in range')
            return param
    
    @staticmethod
    def _check_float_param(param, lo, hi):
        """Check if parameter is float and within given range.
        
        Args:
            param (convertible to float): Parameter to check
            lo (float): Lowest allowed value, or None
            hi (float): Highest allowed value, or None
            
        Returns:
            (float): Parameter converted to float
        """
        
        try:
            param = float(param)            
        except ValueError:
            raise ValueError('Parameter is not float or similar')
        else:
            if lo is None:
                lo = float('-inf')
            if hi is None:
                hi = float('inf')
            if not lo <= param <= hi:
                raise ValueError('Parameter is not in range')
            return param
    
    @staticmethod
    def _input_to_vec(x):
        """Convert input matrix to vector."""
        
        return x.T.reshape(-1, 1)
        
    def adapt(self, d, x):
        """Adapt weights using desired value and its input.
        
        Args:
            d (dim*1 array): Desired value
            x (dim*order array): Input array
        """
        
        t_start_ns = time_ns()

        x_vec = self._input_to_vec(x)
        y = self.wts @ x_vec
        e = d - y
        delta =  self.mu * e * x_vec.T 
        self.wts += delta

        time_us = (time_ns() - t_start_ns) / 1000
        print(f'time (us): {time_us}')

    def predict(self, x):
        """Predict output for given input using current filter state.
        
        Args:
            x (dim*order array): Filter input matrix. Each column is a signal
                value for a given time.
        Returns:
            (dim*1 array): Output vector.
        """
        
        return self.wts @ self._input_to_vec(x)


class FilterAutoLMS:
    """This class implements a LMS filter on an autoregressive process.

    It maintains a history of signal values of size `order`, and uses this
    history as the input for the LMS filter, which is trained to predict the
    next value of the time series.
    
    Args:
        dim (int): Dimension of signal
        order (int): Order of filter
        mu (float, optional): Learning rate. Also known as step size. If it is
            too slow, the filter may have bad performance. If it is too high,
            the filter will be unstable. The default value can be unstable for
            ill-conditioned input data.
        wts_init (str, optional): Method for initializing weights. Options are:
            - 'random': Create random weights
            - 'zeros': Create zero value weights
    """
 

    def __init__(self, dim, order, mu=0.01, wts_init='random'):

        self.flt_lms = FilterLMS(dim, order, mu=mu, wts_init=wts_init)
        self.x_hist = np.zeros((dim, order))

    def predict_next(self, x):
        """Update filter with new signal value and predict next value.

        Args:
            x (dim*1 array): New signal value

        Returns:
            (dim*1 array): Predicted next signal value
        """

        # Update filter using history as input and current value as output
        self.flt_lms.adapt(x, self.x_hist)

        # Add current value to history, dropping oldest value
        self.x_hist = np.concatenate([x, self.x_hist[:, 0:-1]], axis=1)

        # Use new history to predict next value
        return self.flt_lms.predict(self.x_hist)


class FilterAutoEcho:
    """This class implements a dummy 'filter' that returns its input."""

    def __init__(self):
        pass

    def predict_next(self, x):
        """Update filter with new signal value and predict next value.

        Args:
            x (dim*1 array): New signal value

        Returns:
            (dim*1 array): Predicted next signal value
        """

        return x.astype('float64')


def probe_mode(host, port, in_fpath, out_fpath):
    """Run program in mode meant to mimic neuropixel probe."""

    print(f'Loading data from "{in_fpath}"...')
    with h5py.File(in_fpath, 'r') as f:
        spks = f['spks'][:]
        n_neurons = spks.shape[0]
    print('Done.')

    print(f'Connecting to {host}:{port}...')
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    print('Done.')

    print('Sending header information...')

    # Send header
    hdr_bytes = np.array([n_neurons], dtype=np.uint16).tobytes()
    sock.send(hdr_bytes)

    # Receive ACK
    hdr_resp = sock.recv(1)
    if hdr_resp != ACK_BYTE:
        raise ValueError('Response to header not ACK')

    print('Done')

    print('Sending signal...')
    
    filter_preds = np.full((n_neurons, N_PTS), np.nan)
    rt_times_us = np.full(N_PTS, np.nan)

    for i in range(N_PTS):
       
        t_start_ns = time_ns()

        # Write next signal value to client
        sock.send(spks[:, i].tobytes())

        # Read filter prediction from client (64-bit float)
        buf_in = sock.recv(n_neurons * 8)
        filter_preds[:, i] = np.frombuffer(buf_in, dtype=np.float64)

        rt_times_us[i] = (time_ns() - t_start_ns) / 1000

    print('Done.')

    # Summary statistics
    mean_time = np.mean(rt_times_us)
    median_time = np.median(rt_times_us)
    print(f'Mean round-trip latency: {mean_time:.2f} us')
    print(f'Median round-trip latency: {median_time:.2f} us')

    print(f'Writing data to "{out_fpath}"...')
    with h5py.File(out_fpath, 'w') as f:
        f.create_dataset('filter_preds', data=filter_preds)
        f.create_dataset('rt_times_us', data=rt_times_us)
    print('Done.')


def processor_mode(host, port):
    """Run program in mode meant to mimic 'processor' computer reading data"""

    print(f'Starting server at {host}:{port}...')
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((host, port))

    sock.listen(1)
    conn, addr = sock.accept()
    print(f'Connected to probe at {addr}')

    print('Receiving header information...')

    # Read header
    hdr_bytes = conn.recv(HDR_SIZE)
    n_neurons = np.frombuffer(hdr_bytes, dtype=np.uint16)[0]

    # Send ACK
    conn.send(ACK_BYTE)

    print('Done.')

    print('Filtering signal...')

    # Filter for data  
    flt = FilterAutoLMS(n_neurons, FILTER_ORDER, mu=FILTER_MU, wts_init='zeros')
    #flt = FilterAutoEcho()

    while True:

        # Read vector of spike counts from 
        buf_in = conn.recv(n_neurons)
        if not buf_in: 
            break
        x = np.frombuffer(buf_in, dtype=np.uint8).reshape(-1, 1)

        # Run update-predict step on filter
        y = flt.predict_next(x)

        # Send prediction to probe
        conn.send(y.tobytes())

    print('Done.')

    # Close connection
    sock.close()


def parse_args():
    """Parse program command-line arguments."""

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
