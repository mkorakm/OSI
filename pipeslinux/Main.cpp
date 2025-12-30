#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>

using namespace std;

void runProcess(int inputFd, int outputFd, const char* cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        if (inputFd != -1) dup2(inputFd, 0);
        if (outputFd != -1) dup2(outputFd, 1);
        
        // Закрываем лишние дескрипторы
        for (int i = 3; i < 255; i++) close(i);
        
        // Запускаем (cmd передаем дважды, это норма для execl)
        execl(cmd, cmd, NULL);
        exit(1);
    }
}

int main() {
    int p1[2], p2[2], p3[2], p4[2], p5[2];
    
    // Создаем все пайпы
    if (pipe(p1) < 0 || pipe(p2) < 0 || pipe(p3) < 0  || pipe(p4) < 0  ||pipe(p5) < 0) return 1;

    cout << "Launching..." << endl;

    runProcess(p1[0], p2[1], "./M");
    runProcess(p2[0], p3[1], "./A");
    runProcess(p3[0], p4[1], "./P");
    runProcess(p4[0], p5[1], "./S");

    // Родитель закрывает ВСЕ ненужные концы
    close(p1[0]); 
    close(p2[0]); close(p2[1]);
    close(p3[0]); close(p3[1]);
    close(p4[0]); close(p4[1]);
    close(p5[1]);

    string data = "1 2 3\n";
    cout << "Input: " << data;
    write(p1[1], data.c_str(), data.length());
    close(p1[1]); // EOF для M

    char buf[100];
    ssize_t n = read(p5[0], buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = 0;
        cout << "Result: " << buf << endl;
    } else {
        cout << "Result is empty (Error)" << endl;
    }
    
    close(p5[0]);
    return 0;
}
