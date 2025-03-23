#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <log_file_name>" << endl;
        return 1;
    }

    string logFileName = argv[1];
    ofstream logFile(logFileName, ios::app);

    if (!logFile.is_open()) {
        cerr << "Error: Unable to open log file " << logFileName << endl;
        return 1;
    }

    string line;
    while (getline(cin, line)) {
        if (line == "QUIT") {
            break;
        }

        size_t spacePos = line.find_first_of(" \t");
        string action = line.substr(0, spacePos);
        string message = "";

        if (spacePos != string::npos) {
            message = line.substr(spacePos + 1);
        }

        time_t now = time(0);
        struct tm timeInfo;
        localtime_r(&now, &timeInfo);

        logFile << setfill('0') 
                << setw(4) << timeInfo.tm_year + 1900 << "-"
                << setw(2) << timeInfo.tm_mon + 1 << "-"        
                << setw(2) << timeInfo.tm_mday << " "        
                << setw(2) << timeInfo.tm_hour << ":"
                << setw(2) << timeInfo.tm_min << " "
                << "[" << action << "]" << message << endl;
    }

    logFile.close();
    return 0;
}