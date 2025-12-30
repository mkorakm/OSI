#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>    
#include <dirent.h>    
#include <sys/types.h>
#include <signal.h>    
#include <unistd.h>   

using namespace std;

void KillProcessByID(int pid) {

    if (kill(pid, SIGKILL) == 0) {
        cout << "Process " << pid << " killed." << endl;
    } else {
        perror("Failed to kill process");
    }
}

void KillProcessByName(const string& processName) {
    DIR* dir = opendir("/proc");
    if (!dir) return;

    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        
        if (!isdigit(*entry->d_name)) continue;

        int pid = atoi(entry->d_name);
        
        string commPath = string("/proc/") + entry->d_name + "/comm";
        
        ifstream cmdFile(commPath);
        string cmdName;
        if (cmdFile >> cmdName) {
          
            if (cmdName == processName) {
                cout << "Found " << processName << " PID: " << pid << endl;
                KillProcessByID(pid);
            }
        }
    }
    closedir(dir);
}

int main(int argc, char* argv[]) {

    const char* envVar = getenv("PROC_TO_KILL");
    if (envVar) {
        string envStr = envVar;
        stringstream ss(envStr);
        string segment;
        while (getline(ss, segment, ',')) {
            if (!segment.empty()) {
                KillProcessByName(segment);
            }
        }
    }

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--id") {
            if (i + 1 < argc) {
               
                KillProcessByID(stoi(argv[++i]));
            }
        } else if (arg == "--name") {
            if (i + 1 < argc) {
               
                KillProcessByName(argv[++i]);
            }
        }
    }
    return 0;
}
