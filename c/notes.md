# Notes

## TODO

- [x] Basic argument parsing
- [x] Echo server
- [x] Change echo message from string to spike counts
- [x] Change server response from int to float
- [x] Write header exchange in order to allow number of neurons to differ
- [x] Make code work with non-constant number of neurons
- [x] Set up project with CMake and Conan
- [x] Get probe to read from HDF5 file
- [x] Switch dimensions of dataset in HDF5 file (make separate file)
- [x] Get probe mode to save filter predictions
- [x] Add timing
- [ ] Move protocol code into its own file
- [ ] Write echo filter (figure out how to do object-oriented stuff in C)
- [ ] Write LMS filter
- [ ] Get option parsing working so we can change host, port, and filter type

## General

- Will eventually need more standardized way of sending and recieving data, to deal with endianness issues between different machines
- Also need to eventually think about clean way to write protocol on top of TCP, so that header exchange is abstracted away
    - This could be done the way TCP does it, with a `struct` containing connection information
    - `protocol.c` file defines two structs: one for probe connection, other for processor connection
    - It also defines `connect_probe()` and `connect_processor()`, as well as `close_probe()` and `close_processor()` methods
    - Each connection object also has send and recieve methods, which can be used for exchange of data
- Neither of these concerns should matter at the prototyping stage, however
- Probably should consider building C++ prototype; C++ has better support for math
- Looked into possibility of using variable length arrays instead of heap allocation; doesn't seem like common practice
