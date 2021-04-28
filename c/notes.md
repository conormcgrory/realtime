# Notes

## TODO

- [x] Basic argument parsing
- [x] Echo server
- [x] Change echo message from string to spike counts
- [x] Change server response from int to float
- [ ] Write header exchange in order to allow number of neurons to differ
- [ ] Write echo filter (figure out how to do object-oriented stuff in C)
- [ ] Get probe to read from HDF5 file
- [ ] Write LMS filter
- [ ] Get option parsing working so we can change host, port, and filter type

## General

- Will eventually need more standardized way of sending and recieving data, to deal with endianness issues between different machines
- Also need to eventually think about clean way to write protocol on top of TCP, so that header exchange is abstracted away
- Neither of these concerns should matter at the prototyping stage, however
