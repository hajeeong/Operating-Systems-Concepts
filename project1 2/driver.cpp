#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

// Function to check if a string contains only alphabetic characters
bool is_alpha_only(const string& str) {
    return all_of(str.begin(), str.end(), [](char c) { 
        return isalpha(c) || isspace(c); 
    });
}

// Function to write to pipe with error checking
void write_to_pipe(int pipe_fd, const string& message) {
    ssize_t bytes_written = write(pipe_fd, message.c_str(), message.length());
    if (bytes_written < 0) {
        perror("Write to pipe failed");
        exit(1);
    }
}

// Function to read from pipe with error checking
string read_from_pipe(int pipe_fd) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];  // Proper array declaration
    
    ssize_t bytes_read = read(pipe_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Read from pipe failed");
        exit(1);
    }
    
    buffer[bytes_read] = '\0';
    return string(buffer);
}

void display_menu() {
    cout << "\n=== Encryption System Menu ===\n";
    cout << "password - Set encryption password\n";
    cout << "encrypt  - Encrypt a string\n";
    cout << "decrypt  - Decrypt a string\n";
    cout << "history  - Show history of strings\n";
    cout << "quit     - Exit the program\n";
    cout << "==========================\n";
    cout << "Enter command: ";
}

int main(int argc, char* argv[]) {
    // Ensure log file name is provided
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <log_file_name>\n";
        return 1;
    }
    
    string log_file = argv[1];
    vector<string> history;
    
    // Pipes for logger
    int logger_pipe[2];
    if (pipe(logger_pipe) == -1) {
        perror("Logger pipe creation failed");
        return 1;
    }
    
    // Pipes for encryption program
    int encrypt_in_pipe[2];  // Driver writes to encryption
    int encrypt_out_pipe[2]; // Driver reads from encryption
    
    if (pipe(encrypt_in_pipe) == -1 || pipe(encrypt_out_pipe) == -1) {
        perror("Encryption pipe creation failed");
        return 1;
    }
    
    // Fork for logger
    pid_t logger_pid = fork();
    
    if (logger_pid == -1) {
        perror("Logger fork failed");
        return 1;
    }
    
    if (logger_pid == 0) {  // Logger child process
        // Redirect stdin to read from pipe
        close(logger_pipe[1]);  // Close write end
        dup2(logger_pipe[0], STDIN_FILENO);
        close(logger_pipe[0]);
        
        // Execute logger program
        execlp("./logger", "logger", log_file.c_str(), NULL);
        
        // If execlp returns, it failed
        perror("Logger execution failed");
        exit(1);
    }
    
    // Close read end of logger pipe in parent
    close(logger_pipe[0]);
    
    // Fork for encryption program
    pid_t encrypt_pid = fork();
    
    if (encrypt_pid == -1) {
        perror("Encryption fork failed");
        return 1;
    }
    
    if (encrypt_pid == 0) {  // Encryption child process
        // Redirect stdin to read from pipe
        close(encrypt_in_pipe[1]);  // Close write end
        dup2(encrypt_in_pipe[0], STDIN_FILENO);
        close(encrypt_in_pipe[0]);
        
        // Redirect stdout to write to pipe
        close(encrypt_out_pipe[0]);  // Close read end
        dup2(encrypt_out_pipe[1], STDOUT_FILENO);
        close(encrypt_out_pipe[1]);
        
        // Execute encryption program
        execlp("./encryption", "encryption", NULL);
        
        // If execlp returns, it failed
        perror("Encryption execution failed");
        exit(1);
    }
    
    // Close unused pipe ends in parent
    close(encrypt_in_pipe[0]);
    close(encrypt_out_pipe[1]);
    
    // Log the start of the driver program
    string start_log = "START Driver program started";
    write_to_pipe(logger_pipe[1], start_log + "\n");
    
    bool running = true;
    while (running) {
        display_menu();
        
        string command;
        getline(cin, command);
        
        // Convert command to lowercase
        transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        // Log the command
        string command_log = "COMMAND User entered: " + command;
        write_to_pipe(logger_pipe[1], command_log + "\n");
        
        if (command == "password") {
            string password;
            bool use_history = false;
            
            if (!history.empty()) {
                cout << "Use history? (y/n): ";
                string response;
                getline(cin, response);
                
                if (response == "y" || response == "Y") {
                    use_history = true;
                    cout << "\n=== History ===\n";
                    for (size_t i = 0; i < history.size(); i++) {
                        cout << i + 1 << ". " << history[i] << "\n";
                    }
                    cout << "0. Enter new password\n";
                    cout << "Enter selection: ";
                    
                    int selection;
                    cin >> selection;
                    cin.ignore(); // Clear newline
                    
                    if (selection > 0 && selection <= static_cast<int>(history.size())) {
                        password = history[selection - 1];
                    } else {
                        use_history = false;
                    }
                }
            }
            
            if (!use_history) {
                cout << "Enter password (letters only): ";
                getline(cin, password);
                
                // Validate password (letters only)
                if (!is_alpha_only(password)) {
                    cout << "Error: Password must contain only letters\n";
                    continue;
                }
            }
            
            // Send password to encryption program
            string pass_command = "PASS " + password + "\n";
            write_to_pipe(encrypt_in_pipe[1], pass_command);
            
            // Read response
            string response = read_from_pipe(encrypt_out_pipe[0]);
            
            // Log the result (don't log the actual password)
            string result_log = "RESULT Password set";
            write_to_pipe(logger_pipe[1], result_log + "\n");
            
            cout << "Password set successfully\n";
        }
        else if (command == "encrypt" || command == "decrypt") {
            string input_string;
            bool use_history = false;
            
            if (!history.empty()) {
                cout << "Use history? (y/n): ";
                string response;
                getline(cin, response);
                
                if (response == "y" || response == "Y") {
                    use_history = true;
                    cout << "\n=== History ===\n";
                    for (size_t i = 0; i < history.size(); i++) {
                        cout << i + 1 << ". " << history[i] << "\n";
                    }
                    cout << "0. Enter new string\n";
                    cout << "Enter selection: ";
                    
                    int selection;
                    cin >> selection;
                    cin.ignore(); // Clear newline
                    
                    if (selection > 0 && selection <= static_cast<int>(history.size())) {
                        input_string = history[selection - 1];
                    } else {
                        use_history = false;
                    }
                }
            }
            
            if (!use_history) {
                cout << "Enter string (letters only): ";
                getline(cin, input_string);
                
                // Validate input (letters only)
                if (!is_alpha_only(input_string)) {
                    cout << "Error: String must contain only letters\n";
                    continue;
                }
                
                // Add to history
                history.push_back(input_string);
            }
            
            // Convert command to uppercase for encryption program
            string encrypt_command = command;
            transform(encrypt_command.begin(), encrypt_command.end(), encrypt_command.begin(), ::toupper);
            
            // Send command to encryption program
            string full_command = encrypt_command + " " + input_string + "\n";
            write_to_pipe(encrypt_in_pipe[1], full_command);
            
            // Read response
            string response = read_from_pipe(encrypt_out_pipe[0]);
            
            // Display and log the result
            cout << response;
            
            // Parse the response to extract result type and message
            size_t space_pos = response.find(' ');
            string result_type = response.substr(0, space_pos);
            string result_message = "";
            
            if (space_pos != string::npos) {
                result_message = response.substr(space_pos + 1);
                // Remove newline if present
                if (!result_message.empty() && result_message.back() == '\n') {
                    result_message.pop_back();
                }
                
                // Add to history if operation was successful
                if (result_type == "RESULT") {
                    history.push_back(result_message);
                }
            }
            
            // Log the result
            string result_log = result_type + " " + encrypt_command + " operation: " + result_message;
            write_to_pipe(logger_pipe[1], result_log + "\n");
        }
        else if (command == "history") {
            if (history.empty()) {
                cout << "History is empty\n";
            } else {
                cout << "\n=== History ===\n";
                for (size_t i = 0; i < history.size(); i++) {
                    cout << i + 1 << ". " << history[i] << "\n";
                }
            }
            
            // Log the history display
            string history_log = "INFO History displayed";
            write_to_pipe(logger_pipe[1], history_log + "\n");
        }
        else if (command == "quit") {
            running = false;
            
            // Send QUIT to encryption program
            write_to_pipe(encrypt_in_pipe[1], "QUIT\n");
            
            // Send QUIT to logger
            write_to_pipe(logger_pipe[1], "QUIT\n");
            
            // Log the exit
            string exit_log = "EXIT Driver program exiting";
            write_to_pipe(logger_pipe[1], exit_log + "\n");
            
            cout << "Exiting...\n";
        }
        else {
            cout << "Unknown command\n";
            
            // Log the unknown command
            string unknown_log = "ERROR Unknown command: " + command;
            write_to_pipe(logger_pipe[1], unknown_log + "\n");
        }
    }
    
    // Close pipes
    close(encrypt_in_pipe[1]);
    close(encrypt_out_pipe[0]);
    close(logger_pipe[1]);
    
    // Wait for child processes to terminate
    waitpid(logger_pid, NULL, 0);
    waitpid(encrypt_pid, NULL, 0);
    
    return 0;
}