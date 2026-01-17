#include <iostream>
#include <vector>
#include <pthread.h> 
#include <semaphore.h> 
#include <unistd.h>    
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <locale>

using namespace std;
using namespace chrono;


pthread_mutex_t mtx; 
sem_t sem;           

struct ThreadParams {
    int Ai, Bj, At, k, N;
    vector<vector<int>>* A;
    vector<vector<int>>* B;
    vector<vector<int>>* C;
};


void* multiply_block_pthread(void* arg) {
    ThreadParams* p = (ThreadParams*)arg;
    int Ai = p->Ai;
    int Bj = p->Bj;
    int At = p->At;
    int k = p->k;
    int N = p->N;

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
                sum += (*p->A)[rowA + i][colA + x] * (*p->B)[rowB + x][colB + j];
            }

           
            pthread_mutex_lock(&mtx);
            (*p->C)[rowC + i][colC + j] += sum;
            pthread_mutex_unlock(&mtx);
        }
    }

    delete p; 


    sem_post(&sem);
    return NULL;
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
    
    setlocale(LC_ALL, "");

   
    pthread_mutex_init(&mtx, NULL);

   
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores < 1) num_cores = 4;
    

    sem_init(&sem, 0, num_cores);

    int N, k;

    cout << "Size N (>=5): ";
    cin >> N;
    cout << "Block size k (1-" << N << "): ";
    cin >> k;

    if (N < 5 || k < 1 || k > N) {
        cout << "Error input!" << endl;
        return 1;
    }

    vector<vector<int>> A(N, vector<int>(N));
    vector<vector<int>> B(N, vector<int>(N));
    vector<vector<int>> C(N, vector<int>(N, 0));

    srand(time(0));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
    }

    auto start_parallel = high_resolution_clock::now();

    int blocks_num = (N + k - 1) / k;
    vector<pthread_t> threads; 

    for (int i = 0; i < blocks_num; i++) {
        for (int j = 0; j < blocks_num; j++) {
            for (int t = 0; t < blocks_num; t++) {
                ThreadParams* params = new ThreadParams{ i, j, t, k, N, &A, &B, &C };

                sem_wait(&sem);

                pthread_t thread_id;
                
                if (pthread_create(&thread_id, NULL, multiply_block_pthread, params) == 0) {
                    threads.push_back(thread_id);
                } else {
                   
                    sem_post(&sem);
                    delete params;
                }
            }
        }
    }

 
    for (pthread_t th : threads) {
        pthread_join(th, NULL);
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

    cout << "\n=== RESULTS ===" << endl;
    cout << "Matrix: " << N << "x" << N << endl;
    cout << "Block: " << k << "x" << k << endl;
    cout << "Blocks count: " << blocks_num << endl;
    cout << "Threads total: " << threads.size() << endl;
    cout << "Time parallel: " << time_parallel.count() << " us" << endl;
    cout << "Time simple: " << time_simple.count() << " us" << endl;

    if (time_parallel.count() > 0) {
        cout << "Speedup: " << fixed << setprecision(2)
             << (double)time_simple.count() / time_parallel.count() << " x" << endl;
    }

    cout << "Results: " << (correct ? "MATCH" : "MISMATCH") << endl;

    
    if (N <= 100) {
        cout << "\n=== OPTIMIZATION SEARCH ===" << endl;
        cout << "k\tBlocks\tThreads\tTime (us)\tSpeedup" << endl;
        cout << "----------------------------------------------" << endl;

        long long best_time = time_simple.count();
        int best_k = 1;

        vector<int> test_k_values;
        test_k_values.push_back(1);
        test_k_values.push_back(N);

        int step = max(1, N / 10);
        for (int val = step; val < N; val += step) {
            if (val != 1 && val != N) test_k_values.push_back(val);
        }

        for (int test_k : test_k_values) {
            if (test_k < 1 || test_k > N) continue;

            vector<vector<int>> C_test(N, vector<int>(N, 0));
            vector<pthread_t> test_threads;

            auto start = high_resolution_clock::now();
            int test_blocks = (N + test_k - 1) / test_k;

            for (int i = 0; i < test_blocks; i++) {
                for (int j = 0; j < test_blocks; j++) {
                    for (int t = 0; t < test_blocks; t++) {
                        ThreadParams* params = new ThreadParams{ i, j, t, test_k, N, &A, &B, &C_test };
                        
                        sem_wait(&sem);
                        pthread_t th;
                        if (pthread_create(&th, NULL, multiply_block_pthread, params) == 0) {
                            test_threads.push_back(th);
                        } else {
                            sem_post(&sem);
                            delete params;
                        }
                    }
                }
            }

            for (pthread_t th : test_threads) {
                pthread_join(th, NULL);
            }

            auto end = high_resolution_clock::now();
            auto time_test = duration_cast<microseconds>(end - start);
            double speedup = (double)time_simple.count() / time_test.count();

            cout << test_k << "\t" << test_blocks << "\t" << test_threads.size() << "\t"
                 << time_test.count() << "\t\t" << fixed << setprecision(2) << speedup << endl;
        }
    }


    pthread_mutex_destroy(&mtx);
    sem_destroy(&sem);

    return 0;
}
