#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>

using namespace std;

bool IsProcessRunning(const string& processName) {
    DIR* dir = opendir("/proc");
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (!isdigit(*entry->d_name)) continue;

        string pidStr = entry->d_name;
        string commPath = "/proc/" + pidStr + "/comm";
        ifstream commFile(commPath);
        string cmdName;
        
        if (!(commFile >> cmdName)) continue;

        if (cmdName == processName) {
            string statPath = "/proc/" + pidStr + "/stat";
            ifstream statFile(statPath);
            
            string trash;
            string state;
            
            statFile >> trash; 
            statFile >> trash; 
            statFile >> state;

            if (state != "Z") {
                closedir(dir);
                return true; 
            }
        }
    }
    closedir(dir);
    return false;
}

pid_t StartTestProcess(const string& appName) {
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    } else if (pid == 0) {
        execlp(appName.c_str(), appName.c_str(), "100", NULL);
        exit(1); 
    }
    return pid;
}

void RunKiller(const string& argType, const string& argValue) {
    pid_t pid = fork();
    if (pid == 0) {
        if (argType.empty()) {
            execl("./killer", "killer", NULL);
        } else {
            execl("./killer", "killer", argType.c_str(), argValue.c_str(), NULL);
        }
        exit(1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

int main() {
    string victim = "sleep"; 

    setenv("PROC_TO_KILL", "sleep,vlc", 1);

    pid_t pid1 = StartTestProcess(victim);
    usleep(100000); 

    if (IsProcessRunning(victim)) cout << "1. Process running." << endl;
    
    RunKiller("", "");
    usleep(100000); 

    if (!IsProcessRunning(victim)) cout << "1. Success: Process killed." << endl;
    else cout << "1. Fail." << endl;

    if (pid1 > 0) waitpid(pid1, nullptr, WNOHANG);
    unsetenv("PROC_TO_KILL");

    pid_t pid2 = StartTestProcess(victim);
    usleep(100000);

    RunKiller("--name", victim);
    usleep(100000);

    if (!IsProcessRunning(victim)) cout << "2. Success: Process killed by name." << endl;
    else cout << "2. Fail." << endl;
    
    if (pid2 > 0) waitpid(pid2, nullptr, WNOHANG);

    pid_t pid3 = StartTestProcess(victim);
    usleep(100000);

    RunKiller("--id", to_string(pid3));
    usleep(100000);

    if (!IsProcessRunning(victim)) cout << "3. Success: Process killed by ID." << endl;
    else cout << "3. Fail." << endl;
    
    if (pid3 > 0) waitpid(pid3, nullptr, WNOHANG);

    return 0;
}
