# realtime

Real-time data analysis project for CATNIP lab

## Directory structure

This directory contains code for a simple version of the real-time filtering system. The directory layout is as follows:

- `data`: Data used to test prototype. Contains two subdirectories:
    - `raw`: Raw Steinmetz data downloaded from server 
    - `processed`: Data derived from Steinmetz data
    - `results`: Results from latency experiments
- `notebooks`: Jupyter notebooks used to analyze experiment results
- `julia`: Julia implementation of real-time filtering prototype
- `python`: Python implementation of real-time filtering prototype
- `rust`: Rust implementation of real-time filtering prototype
- `c`: C implementation of real-time filtering prototype

## Setup

### Requirements

This project requires installations of Python 3, Rust, Julia, Conan, and CMake.
