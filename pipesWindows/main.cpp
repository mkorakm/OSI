//Main -> Pipe1 -> M -> Pipe2 -> A -> Pipe3 -> P -> Pipe4 -> S -> Pipe5 -> Main

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

bool RunProcess(string exeName, HANDLE hInput, HANDLE hOutput) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES; 
    si.hStdInput = hInput;
    si.hStdOutput = hOutput;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    ZeroMemory(&pi, sizeof(pi));

    string cmdLine = exeName;

    if (!CreateProcessA(NULL, &cmdLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        cerr << "Failed to start process: " << exeName << " Error: " << GetLastError() << endl;
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int main() {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; 
    sa.lpSecurityDescriptor = NULL;

    HANDLE hPipeM_In_R, hPipeM_In_W;   // Вход в M
    HANDLE hPipeA_In_R, hPipeA_In_W;   // M -> A
    HANDLE hPipeP_In_R, hPipeP_In_W;   // A -> P
    HANDLE hPipeS_In_R, hPipeS_In_W;   // P -> S
    HANDLE hPipeS_Out_R, hPipeS_Out_W; // Выход из S в Main

    if (!CreatePipe(&hPipeM_In_R, &hPipeM_In_W, &sa, 0) ||
        !CreatePipe(&hPipeA_In_R, &hPipeA_In_W, &sa, 0) ||
        !CreatePipe(&hPipeP_In_R, &hPipeP_In_W, &sa, 0) ||
        !CreatePipe(&hPipeS_In_R, &hPipeS_In_W, &sa, 0) ||
        !CreatePipe(&hPipeS_Out_R, &hPipeS_Out_W, &sa, 0)) {
        cerr << "Pipe creation failed." << endl;
        return 1;
    }

    SetHandleInformation(hPipeM_In_W, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hPipeS_Out_R, HANDLE_FLAG_INHERIT, 0);

    cout << "Launching processes..." << endl;


    if (!RunProcess("M.exe", hPipeM_In_R, hPipeA_In_W)) return 1;
    SetHandleInformation(hPipeA_In_W, HANDLE_FLAG_INHERIT, 0);

    if (!RunProcess("A.exe", hPipeA_In_R, hPipeP_In_W)) return 1;
    SetHandleInformation(hPipeP_In_W, HANDLE_FLAG_INHERIT, 0);


    if (!RunProcess("P.exe", hPipeP_In_R, hPipeS_In_W)) return 1;
    SetHandleInformation(hPipeS_In_W, HANDLE_FLAG_INHERIT, 0);

    if (!RunProcess("S.exe", hPipeS_In_R, hPipeS_Out_W)) return 1;

    
    CloseHandle(hPipeM_In_R);
    CloseHandle(hPipeA_In_W);
    CloseHandle(hPipeA_In_R);
    CloseHandle(hPipeP_In_W);
    CloseHandle(hPipeP_In_R);
    CloseHandle(hPipeS_In_W);
    CloseHandle(hPipeS_In_R);
    CloseHandle(hPipeS_Out_W);

    string inputData = "1 2 3"; 
    cout << "Input data: " << inputData << endl;

    DWORD bytesWritten;
    WriteFile(hPipeM_In_W, inputData.c_str(), (DWORD) inputData.length(), &bytesWritten, NULL);

    CloseHandle(hPipeM_In_W);

    char buffer[128];
    DWORD bytesRead;
    string result = "";

    while (ReadFile(hPipeS_Out_R, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }
    CloseHandle(hPipeS_Out_R);

    cout << "Result form pipeline: " << result << endl;

    return 0;
}