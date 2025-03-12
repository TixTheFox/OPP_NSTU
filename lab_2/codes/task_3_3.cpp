#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
#include <chrono>
#include <cmath>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }    

#define EXP_NUM 50
#define EXP_SIZE 1e7

#define SPIN

#ifdef SPIN
pthread_spinlock_t spin;
#else
pthread_mutex_t mutex;
#endif

chrono::time_point<chrono::high_resolution_clock> t1, t2;

void* main_thread(void* arg) {
    #ifdef SPIN
    pthread_spin_lock(&spin);
    #else
    pthread_mutex_lock(&mutex);
    #endif

    t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < EXP_SIZE; i++) sin(i);
    t2 = chrono::high_resolution_clock::now();

    #ifdef SPIN
    pthread_spin_unlock(&spin);
    #else
    pthread_mutex_unlock(&mutex);
    #endif
   
    pthread_exit(NULL);
}

void* side_thread(void* arg) {
    #ifdef SPIN
    pthread_spin_lock(&spin);
    pthread_spin_unlock(&spin);
    #else
    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);
    #endif
   
   pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // проверки введенного аргумента
    if (argc != 2) {
        cout << "Недопустимое число аргументов. Введите один аргумент - число потоков, которые будут запущены для теста\n";
        return -1;
    }
    
    int thread_count = 0;

    if ( ((thread_count = atoi(argv[1])) == 0) || thread_count <= 0) {
        cout << "Число потоков должно быть целым положительным числом\n";
        return -1;
    }

    #ifdef SPIN
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    #else
    pthread_mutex_init(&mutex, NULL);
    #endif
    

    pthread_t m_thread;                                 // основной поток
    pthread_t* threads = new pthread_t[thread_count];   // сторонние потоки

    int err;

    double time_result = 0;

    for (int exp = 0; exp < EXP_NUM; exp++) {

        err = pthread_create(&m_thread, NULL, main_thread, NULL);
        if (err != 0) err_exit(err, "Невозможно создать поток");

        for (int i = 0; i < thread_count; i++) {
            err = pthread_create(&threads[i], NULL, side_thread, NULL);
            if (err != 0) err_exit(err, "Невозможно создать поток");
        }

        pthread_join(m_thread, NULL);
        for (int i = 0; i < thread_count; i++) {
            pthread_join(threads[i], NULL);
        }

        time_result += chrono::duration<double>(t2 - t1).count();
    }
    time_result /= EXP_NUM;

    cout << "Time: " << time_result << endl;

    #ifdef SPIN
    pthread_spin_destroy(&spin);
    #else
    pthread_mutex_destroy(&mutex);
    #endif
    
    delete threads;
}