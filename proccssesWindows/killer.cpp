#undef UNICODE
#undef _UNICODE

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void KillProcessByID(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        return;
    }
    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

void KillProcessByName(const string& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe)) {
        CloseHandle(hSnapshot);
        return;
    }

    do {
        if (string(pe.szExeFile) == processName) {
            cout << "Found " << processName << " PID: " << pe.th32ProcessID << endl;
            KillProcessByID(pe.th32ProcessID);
        }
    } while (Process32Next(hSnapshot, &pe));

    CloseHandle(hSnapshot);
}

int main(int argc, char* argv[]) {
    const int bufferSize = 32767;
    char envBuffer[bufferSize];

    if (GetEnvironmentVariable("PROC_TO_KILL", envBuffer, bufferSize) > 0) {
        string envVal = envBuffer;
        stringstream ss(envVal);
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
                DWORD pid = static_cast<DWORD>(stol(argv[++i]));
                KillProcessByID(pid);
            }
        }
        else if (arg == "--name") {
            if (i + 1 < argc) {
                string name = argv[++i];
                KillProcessByName(name);
            }
        }
    }
    return 0;
}