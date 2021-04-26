# Notes

## TODO

- [x] Basic argument parsing
- [x] Echo server
- [x] Change echo message from string to spike counts
- [ ] Change server response from int to float
- [ ] Write echo filter (figure out how to do object-oriented stuff in C)
- [ ] Write header exchange in order to allow number of neurons to differ
- [ ] Get probe to read from HDF5 file
- [ ] Write LMS filter

## General

- Will eventually need more standardized way of sending and recieving data, to deal with endianness issues between different machines
- Also need to eventually think about clean way to write protocol on top of TCP, so that header exchange is abstracted away
