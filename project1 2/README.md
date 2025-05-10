# Encryption System Project

This project implements a system of three inter-connected programs for text encryption and decryption using the Vigenère cipher.
## Project Files

- driver.cpp - Main program that interacts with the user and launches the other programs
- encryption.cpp - Program that handles the encryption and decryption operations
- logger.cpp - Program that logs all system activities with timestamps
- Makefile - Used to compile all programs
- devlog.md - Development log tracking project progress

## Compilation Instructions
To compile all programs, simply run:
```
make
```
This will create three executables: ```driver```, ```encryption```, and ```logger```.
To clean the compiled files:
```
make clean
```
## Running the Program
To run the encryption system, execute the driver program with a log file name:
```
./driver logfile.txt
```

The driver will automatically start the logger and encryption programs and connect them using pipes.

## Program Usage
After starting the driver, you'll see a menu with the following options:

- ```password``` - Set the encryption password
- ```encrypt``` - Encrypt a string
- ```decrypt``` - Decrypt a string
- ```history``` - Show history of strings used and results
- ```quit``` - Exit the program

## Notes

- All inputs for encryption, decryption, and passwords must contain only letters.
- The system implements a case-insensitive Vigenère cipher.
- All activities are logged to the specified log file.
- The history feature allows reusing previously entered strings.