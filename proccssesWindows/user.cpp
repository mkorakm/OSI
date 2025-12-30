#undef UNICODE
#undef _UNICODE

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>

using namespace std;

bool IsProcessRunning(const string& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (string(pe.szExeFile) == processName) {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return false;
}

PROCESS_INFORMATION StartTestProcess(const string& appName) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    string cmdNonConst = appName;

    CreateProcess(NULL, &cmdNonConst[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    return pi;
}

void RunKiller(const string& args) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    string cmd = "Killer.exe " + args;

    if (CreateProcess(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int main() {
    string victim = "notepad.exe";

    SetEnvironmentVariable("PROC_TO_KILL", "notepad.exe,calc.exe");

    PROCESS_INFORMATION pi1 = StartTestProcess(victim);
    Sleep(1000);

    if (IsProcessRunning(victim)) cout << "1. Process running." << endl;
    else cout << "1. Process NOT running." << endl;

    RunKiller("");

    if (!IsProcessRunning(victim)) cout << "1. Success: Process killed." << endl;
    else cout << "1. Fail: Process alive." << endl;

    CloseHandle(pi1.hProcess);
    CloseHandle(pi1.hThread);

    SetEnvironmentVariable("PROC_TO_KILL", NULL);

    PROCESS_INFORMATION pi2 = StartTestProcess(victim);
    Sleep(1000);

    RunKiller("--name " + victim);

    if (!IsProcessRunning(victim)) cout << "2. Success: Process killed by name." << endl;
    else cout << "2. Fail: Process alive." << endl;

    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);

    PROCESS_INFORMATION pi3 = StartTestProcess(victim);
    Sleep(1000);

    RunKiller("--id " + to_string(pi3.dwProcessId));

    if (!IsProcessRunning(victim)) cout << "3. Success: Process killed by ID." << endl;
    else cout << "3. Fail: Process alive." << endl;

    CloseHandle(pi3.hProcess);
    CloseHandle(pi3.hThread);

    SetEnvironmentVariable("PROC_TO_KILL", NULL);

    system("pause");
    return 0;
}