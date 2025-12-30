#include <iostream>

int main() {
    int N = 8;
    long long x;
    while (std::cin >> x) {
        std::cout << (x + N) << " ";
    }
    return 0;
}