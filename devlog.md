# Development Log
## 2025-03-22 6:01pm

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

