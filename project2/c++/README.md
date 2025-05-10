# Bank Simulation Project

## Description
This project simulates a bank with tellers serving customers. It uses multithreading and semaphores to manage concurrent access to shared resources like the safe, manager, and customer queue.

## Files
- `bank_simulation.cpp` - Main simulation code containing the bank logic
- `semaphore.h` - Header file for the Semaphore class
- `semaphore.cpp` - Implementation of the Semaphore class
- `Makefile` - Compilation instructions

## Compilation
To compile the project, use:
```bash
make bank_simulation
```
Or manually:
```
g++ --std=c++11 -lpthread semaphore.cpp bank_simulation.cpp -o bank_simulation
```

### Running the Program
```
./bank_simulation
```

## Features

- Simulates 3 tellers and 50 customers
- Customers can perform deposit or withdrawal transactions
- Safe has limited capacity (2 tellers at once)
- Withdrawals require manager approval
- Uses semaphores for thread synchronization

## Requirements

- C++11 or higher
0 pthread library