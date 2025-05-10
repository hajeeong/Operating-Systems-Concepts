# Development Log
## 2025-03-10 10:00

### Initial Thoughts 

I'm starting a project that involves creating a system of three interconnected programs:
1. A logger that timestamps and records all activities
2. An encryption program that implements the Vigenere cipher
3. A driver program that coordinates these two programs and interacts with the user


The programs will communicate via pipes, with the driver launching both the logger and encryption program as separate processes. This is a good exercise in inter-process communication and system programming.

The Vigenère cipher is a method of encrypting text using a series of interwoven Caesar ciphers, based on the letters of a keyword. I'll need to implement both encryption and decryption functions.

For this project, I plan to use C++ as it provides good control over system calls like fork(), pipe(), and dup2() which are required for the inter-process communication.

### Initial Plan

1. Create the basic structure for all three programs
2. Implement the logger program first as it's the simplest
3. Implement the encryption program with the Vigenère cipher algorithm
4. Create the driver program to coordinate everything
5. Test extensively with various inputs and edge cases

I'll track my progress in this development log and make regular commits to show the evolution of the project.

## 2025-03-18 14:30

### Thoughts So Far

After reviewing the project requirements in more detail, I have a better understanding of how the three programs need to interact. The driver program will create two child processes using fork() and connect them using pipes. This will allow for bidirectional communication between the driver and the encryption program, as well as unidirectional communication to the logger.

I've also been thinking about the Vigenère cipher implementation. Since it's case-insensitive, I'll convert all input to uppercase for simplicity before processing. I need to ensure that the cipher only modifies alphabetic characters and leaves other characters (spaces, punctuation) unchanged.

### Plan for This Session

1. Implement the logger program with proper timestamp formatting
2. Implement the encryption program with the Vigenère cipher algorithm
3. Start working on the driver program's basic structure
4. Set up the pipes for inter-process communication

My goal for this session is to have working versions of the logger and encryption programs, and at least the skeleton of the driver program.

## 2025-03-22 18:45
### Session Reflection

I've completed the implementation of all three programs:

1. The logger program now properly formats timestamps and logs messages to the specified file.
2. The encryption program implements the Vigenère cipher for both encryption and decryption, handling errors like missing passwords.
3. The driver program successfully:
   - Creates child processes for both the logger and encryption programs
   - Sets up pipes for communication
   - Provides a user interface with all required commands
   - Maintains a history of strings and results
   - Validates user input to ensure only alphabetic characters are used

I also created a Makefile to simplify compilation and a README.md file with usage instructions.


There were some challenges with pipe communication and process management, but I was able to resolve them by carefully reading the documentation for fork(), 
pipe(), and dup2(). I also had to make sure that I properly closed unused pipe ends in each process to avoid potential deadlocks.
For the next session, I plan to:

1. Add more error handling for edge cases
2. Improve the user interface with clearer prompts and feedback
3. Test the system thoroughly with various inputs
4. Clean up any remaining bugs or issues