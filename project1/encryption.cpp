#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

using namespace std;

string vingener_encrypt(const string& text, const string& key) {
    string result = "";
    size_t keyIndex = 0;

    for (char c : text) {
        if (isalpha(c)) {
            // Convert to uppercase for uniformity 
            char base = 'A';
            char upperC = toupper(c);

            // Apply Vingenere encryption (text_char + key_char) mod 26
            char encryptedChar = ((upperC - base) + (toupper(key[keyIndex]) - base)) % 26 + base;

            result += encryptedChar;
            keyIndex = (keyIndex + 1) % key.length();
        } else {
            // Non alphabetic characters remain unchanged
            result += c;
        }
    }
    
    return result;
}

string vigenere_decrypt(const string& text, const string& key) {
    string result = "";
    size_t keyIndex = 0;

    for (char c : text) {
        if (isalpha(c)) {
            // Convert to uppercase for uniformity
            char base = 'A';
            char upperC = toupper(c);

            // Apply Vingenere decryption (text_char - key_char + 26) mod 26
            char decryptedChar = ((upperC - base) - (toupper(key[keyIndex]) - base) + 26) % 26 + base;

            result += decryptedChar;
            keyIndex = (keyIndex + 1) % key.length();
        } else {
            // Non alphabetic characters remain unchanged
            result += c;
        }
    }

    return result;
}

bool is_alpha_only(const string& str) {
    return all_of(str.begin(), str.end(), [](char c) {
        return isalpha(c) || isspace(c);
    });
}

int main() {
    string passkey = "";
    string command, argument;

    while (true) {
        string line;
        if (!getline(cin, line)) {
            break;
        }

        size_t spacePos = line.find_first_of(" \t");
        command = line.substr(0, spacePos);
        argument = "";

        if (spacePos != string::npos) {
            argument = line.substr(spacePos + 1);
        }

        transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "PASS" || command == "PASSKEY") {
            passkey = argument;
            cout << "RESULT" << endl;
        }
        else if (command == "ENCRYPT") {
            if (passkey.empty()) {
                cout << "ERROR: Password not set" << endl;
            } else if (!is_alpha_only(argument)) {
                cout << "ERROR Input must contain only letters" << endl;
            } else {
                string encrypted = vingener_encrypt(argument, passkey);
                cout << "RESULT: " << encrypted << endl;
            }
        }
        else if (command == "DECRYPT") {
            if (passkey.empty()) {
                cout << "ERROR: Password not set" << endl;
            } else if (!is_alpha_only(argument)) {
                cout << "ERROR Input must contain only letters" << endl;
            } else {
                string decrypted = vigenere_decrypt(argument, passkey);
                cout << "RESULT " << decrypted << endl;
            }
        }
        else if (command == "QUIT") {
            break;
        }
        else {
            cout << "ERROR: Invalid command" << endl;
        }
    }

    return 0;
}