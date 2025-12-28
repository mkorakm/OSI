#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <iomanip>
#include <locale>

using namespace std;
using namespace chrono;

mutex mtx;

void multiply_block(int Ai, int Bj, int At, int k, int N,
    const vector<vector<int>>& A,
    const vector<vector<int>>& B,
    vector<vector<int>>& C) {

    int rowA = Ai * k;
    int colA = At * k;
    int rowB = At * k;
    int colB = Bj * k;
    int rowC = Ai * k;
    int colC = Bj * k;

    for (int i = 0; i < k && rowA + i < N; i++) {
        for (int j = 0; j < k && colB + j < N; j++) {
            int sum = 0;
            for (int x = 0; x < k && colA + x < N && rowB + x < N; x++) {
                sum += A[rowA + i][colA + x] * B[rowB + x][colB + j];
            }

            lock_guard<mutex> lock(mtx);
            C[rowC + i][colC + j] += sum;
        }
    }
}

void simple_multiply(const vector<vector<int>>& A,
    const vector<vector<int>>& B,
    vector<vector<int>>& C,
    int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, "rus");

    int N, k;

    cout << "Размер матрицы N (>=5): ";
    cin >> N;
    cout << "Размер блока k (1-" << N << "): ";
    cin >> k;

    if (N < 5 || k < 1 || k > N) {
        cout << "Ошибка ввода!" << endl;
        return 1;
    }

    vector<vector<int>> A(N, vector<int>(N));
    vector<vector<int>> B(N, vector<int>(N));
    vector<vector<int>> C(N, vector<int>(N, 0));

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
    }

    auto start_parallel = high_resolution_clock::now();

    int blocks_num = (N + k - 1) / k;
    vector<thread> threads;

    for (int i = 0; i < blocks_num; i++) {
        for (int j = 0; j < blocks_num; j++) {
            for (int t = 0; t < blocks_num; t++) {
                threads.emplace_back(multiply_block,
                    i, j, t, k, N,
                    ref(A), ref(B), ref(C));
            }
        }
    }

    for (auto& th : threads) {
        th.join();
    }

    auto end_parallel = high_resolution_clock::now();
    auto time_parallel = duration_cast<microseconds>(end_parallel - start_parallel);

    vector<vector<int>> C_simple(N, vector<int>(N, 0));
    auto start_simple = high_resolution_clock::now();
    simple_multiply(A, B, C_simple, N);
    auto end_simple = high_resolution_clock::now();
    auto time_simple = duration_cast<microseconds>(end_simple - start_simple);

    bool correct = true;
    for (int i = 0; i < N && correct; i++) {
        for (int j = 0; j < N; j++) {
            if (C[i][j] != C_simple[i][j]) {
                correct = false;
                break;
            }
        }
    }

    cout << "\n=== РЕЗУЛЬТАТЫ ===" << endl;
    cout << "Матрица: " << N << "x" << N << endl;
    cout << "Блок: " << k << "x" << k << endl;
    cout << "Блоков: " << blocks_num << endl;
    cout << "Потоков: " << threads.size() << endl;
    cout << "Время с потоками: " << time_parallel.count() << " мкс" << endl;
    cout << "Время без потоков: " << time_simple.count() << " мкс" << endl;

    if (time_parallel.count() > 0) {
        cout << "Ускорение: " << fixed << setprecision(2)
            << (double)time_simple.count() / time_parallel.count() << " раз" << endl;
    }

    cout << "Результаты " << (correct ? "совпадают" : "не совпадают") << endl;

    if (N <= 100) {
        cout << "\n=== ПОИСК ОПТИМАЛЬНОГО РАЗМЕРА БЛОКА ===" << endl;
        cout << "k\tБлоков\tПотоков\tВремя (мкс)\tУскорение" << endl;
        cout << "----------------------------------------------" << endl;

        int best_k = 1;
        long long best_time = time_simple.count();
        int best_blocks = 0;
        int best_threads_count = 0;

        vector<int> test_k_values;
        test_k_values.push_back(1);
        test_k_values.push_back(N);

        int step = max(1, N / 10);
        for (int val = step; val < N; val += step) {
            if (val != 1 && val != N) {
                test_k_values.push_back(val);
            }
            if (test_k_values.size() >= 15) break;
        }

        for (int test_k : test_k_values) {
            if (test_k < 1 || test_k > N) continue;

            vector<vector<int>> C_test(N, vector<int>(N, 0));
            vector<thread> test_threads;

            auto start = high_resolution_clock::now();

            int test_blocks = (N + test_k - 1) / test_k;

            for (int i = 0; i < test_blocks; i++) {
                for (int j = 0; j < test_blocks; j++) {
                    for (int t = 0; t < test_blocks; t++) {
                        test_threads.emplace_back(multiply_block,
                            i, j, t, test_k, N,
                            ref(A), ref(B), ref(C_test));
                    }
                }
            }

            for (auto& th : test_threads) {
                th.join();
            }

            auto end = high_resolution_clock::now();
            auto time_test = duration_cast<microseconds>(end - start);

            double speedup = (double)time_simple.count() / time_test.count();
            int threads_count = test_threads.size();

            cout << test_k << "\t" << test_blocks << "\t" << threads_count << "\t"
                << time_test.count() << "\t\t" << fixed << setprecision(2)
                << speedup << endl;

            if (time_test.count() < best_time) {
                best_time = time_test.count();
                best_k = test_k;
                best_blocks = test_blocks;
                best_threads_count = threads_count;
            }
        }

        cout << "\n=== ОПТИМАЛЬНЫЙ РАЗМЕР БЛОКА ===" << endl;
        cout << "Размер блока: " << best_k << endl;
        cout << "Количество блоков: " << best_blocks << endl;
        cout << "Количество потоков: " << best_threads_count << endl;
        cout << "Минимальное время: " << best_time << " мкс" << endl;
        cout << "Максимальное ускорение: " << fixed << setprecision(2)
            << (double)time_simple.count() / best_time << " раз" << endl;
    }

    return 0;
}