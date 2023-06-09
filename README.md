# Basic Shell
## Overview

Basic Shell is, well, a basic shell that supports running commands with flags and input/output file redirection. Multiple commands can be chained together using pipes. The implementation uses c++. The shell outputs the exit status of each command in the pipeline. `255` is used as an exit code indicating a return from execv (meaning binary failed to execute). `exit` is a special command that exits the terminal.

## Examples
```sh
/bin/ls -a
/bin/wc < Makefile > test.txt
/bin/cat main.cc | /bin/grep public
```
## Makefile
Compiles and creates the `msh` executable
```
make
make all
```
Deletes generated object files and the executable
```
make clean
```
### Relevant Readings
- Chapter 5: Process API - [Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)
