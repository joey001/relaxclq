# MKP: Maximal K-plex Computation

This repository implements maximal k-plex computation algorithms which can also computes maximum k-plex, and when k is equal to exact 1, the k-plex is the clique.
## Compile the code

```sh
$ make clean
$ make
```
It generates an executable "MKP".

## Run the code

```sh
$ ./MKP -f {file_path} -m {type} -b -k {k-value} -t {time_limit} -l {lower_bound}
```
An example of computing the exact maximum 15-plex for the dataset soc-slashdot  with time limit 1 second and lower bound 59 is as follows
```sh
$ ./MKP -f soc-slashdot.bin -m "Maximum" -b -k 15 -t 1 -l 59
```

the order of the options are not required, and only filePath and type are required.

## Options information

The most important command line options are:

    -f Path to the input file graph
    -b Convert a text file to a binary file, no argument
    -t time limit, seconds
    -k k-value,default 1
    -m if need to find Maximum, argument is "Maximum", and if need to enumerate maximal, argument is "Maximal"
    -l lower bound, if type is Maximum, default 2k-1, and can not lower than default
                    if type is Maximal, default 0, and can not lower than default
    -h help, list all options
## Data format

We both adopt the time-efficient binary format and the text format, and our code will convert the text file to the binary file automatically with the option -b, while generates an binary file at the same time.
