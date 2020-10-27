"""Client"""

import socket
from time import time_ns

import h5py
import numpy as np


SERVER_HOST = 'localhost'
SERVER_PORT = 2000


def main():

    # Create socket and connect to server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER_HOST, SERVER_PORT))
    
    while True:

        data = sock.recv(8)
        if not data: break
        x = np.frombuffer(data)[0]
        sock.send(x.tobytes())


if __name__ == '__main__':
    main()
