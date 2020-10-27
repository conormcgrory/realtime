"""Script for fetching Steinmetz data from server."""

import os
import requests


# Directory where files are stored
DATA_DIR = '/Users/cmcgrory/park_lab/realtime/basic_prototype/data/raw'

# Filenames and URLs to download files from
FILE_URLS = [
    ('steinmetz_part0.npz', 'https://osf.io/agvxh/download'), 
    ('steinmetz_part1.npz', 'https://osf.io/uv3mw/download'), 
    ('steinmetz_part2.npz', 'https://osf.io/ehmw2/download'),
]


def fetch(url, fpath):
    """Download file from URL to specified path."""
    
    try:
        r = requests.get(url)
    except requests.ConnectionError:
        print('Failed to download data!')
    else:
        if r.status_code != requests.codes.ok:
            print('Failed to download data!')
        else:
            with open(fpath, 'wb') as f:
                f.write(r.content)
   

def main():
    
    print(f'Data directory: {DATA_DIR}')
    
    for fname, url in FILE_URLS:
        
        fpath = os.path.join(DATA_DIR, fname)
        
        if os.path.isfile(fpath):
            print(f"File '{fname}' already exists")
        else:
            print(f"File '{fname}' not found.")
            print(f'Downloading from {url}...')
            fetch(url, fpath)
            print('Done.')

            
if __name__ == '__main__':
    main()