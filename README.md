# MIPS-Interpreter A4
A MIPS interpreter in C++ which models DRAM by using a row buffer, and implements non-blocking memory by executing some instructions in parallel with lw/sw

### Compiling and Executing

After cloning the repository run 

```bash
 cd COL216-A4
 make # To compile col216a4.cpp
 ```
 To run the executable  ```.\a.out``` :
 
 ```
 .\a.out in.txt ROW_ACCESS_DELAY COL_ACCESS_DELAY 
 ```
 Here , ROW_ACCESS_DELAY,COL_ACCESS_DELAY  are integer parameters that must be supplied. 

### Cleaning
```
make clean #this removes the executable ./a.out from the directory
```
    
