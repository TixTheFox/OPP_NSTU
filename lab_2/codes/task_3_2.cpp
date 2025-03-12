#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
#include <chrono>
#include <cmath>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }    


#define EXP_NUM 50      // число экспериментов (берется среднее время за число экспериментов)
#define TASK_SIZE 1e5   // число операций вычислений синуса в одном потоке эксперимента

pthread_mutex_t mutex;
pthread_spinlock_t spinlock;

// функция потока, использующая мьютекс для блокировок
void* thread_mutex(void* arg) {
    int err;

    err = pthread_mutex_lock(&mutex);
    if (err != 0) err_exit(err, "Невозможно захватить мьютекс")

    for (int i = 0; i < TASK_SIZE; i++) sin(i);

    err = pthread_mutex_unlock(&mutex);
    if (err != 0) err_exit(err, "Невозможно освободить мьютекс")

    pthread_exit(NULL);
}

// функция потока, использующая спинлок для блокировок
void* thread_spinlock(void* arg) {
    int err;

    err = pthread_spin_lock(&spinlock);
    if (err != 0) err_exit(err, "Невозможно захватить спинлок")

    for (int i = 0; i < TASK_SIZE; i++) sin(i);

    err = pthread_spin_unlock(&spinlock);
    if (err != 0) err_exit(err, "Невозможно освободить спинлок")

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


    // инициализация мьютекса и спинлока
    pthread_mutex_init(&mutex, NULL);
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);

    // переменные для хранения результатов измерений
    double mutex_time = 0;
    double spin_time = 0;

    // переменные для хранения текущего момента времени
    chrono::time_point<chrono::high_resolution_clock> t1, t2;

    // Код ошибки
    int err;
    // Массив создаваемых потоков
    pthread_t* threads = new pthread_t[thread_count];

    // ----- ТЕСТИРОВАНИЕ МЬЮТЕКСА

    for (int exp = 0; exp < EXP_NUM; exp++) {
        // начало теста
        t1 = chrono::high_resolution_clock::now();
        // создаем потоки
        for (int i = 0; i < thread_count; i++) {
            err = pthread_create(&threads[i], NULL, thread_mutex, NULL);
            if (err != 0) err_exit(err, "Невозможно создать поток");
        }
        // ждем их завершения
        for (int i = 0; i < thread_count; i++) {
            pthread_join(threads[i], NULL);
        }
        // конец теста
        t2 = chrono::high_resolution_clock::now();
        mutex_time += chrono::duration<double>(t2 - t1).count();
    }
    mutex_time /= EXP_NUM;

    // ----- ТЕСТИРОВАНИЕ СПИНЛОКА

    for (int exp = 0; exp < EXP_NUM; exp++) {
        // начало теста
        t1 = chrono::high_resolution_clock::now();
        // создаем потоки
        for (int i = 0; i < thread_count; i++) {
            err = pthread_create(&threads[i], NULL, thread_spinlock, NULL);
            if (err != 0) err_exit(err, "Невозможно создать поток");
        }
        // ждем их завершения
        for (int i = 0; i < thread_count; i++) {
            pthread_join(threads[i], NULL);
        }
        // конец теста
        t2 = chrono::high_resolution_clock::now();
        spin_time += chrono::duration<double>(t2 - t1).count();
    }
    spin_time /= EXP_NUM;

    cout << mutex_time << endl;
    cout << spin_time << endl;

    pthread_mutex_destroy(&mutex);
    pthread_spin_destroy(&spinlock);
    delete threads;
}


    // // измерение времени работы мьютекса
    // for (int i = 0; i < EXP_NUM; i++) {
    //     t1 = chrono::high_resolution_clock::now();
    //     pthread_mutex_lock(&mutex);
    //     t2 = chrono::high_resolution_clock::now();
    //     mutex_lock_time += chrono::duration<double>(t2 - t1).count();

    //     t1 = chrono::high_resolution_clock::now();
    //     pthread_mutex_unlock(&mutex);
    //     t2 = chrono::high_resolution_clock::now();
    //     mutex_unlock_time += chrono::duration<double>(t2 - t1).count();
    // }
    // // берем среднее значение
    // mutex_lock_time /= EXP_NUM;
    // mutex_unlock_time /= EXP_NUM;

    // // измерение времени работы спинлока
    // for (int i = 0; i < EXP_NUM; i++) {
    //     t1 = chrono::high_resolution_clock::now();
    //     pthread_spin_lock(&spinlock);
    //     t2 = chrono::high_resolution_clock::now();
    //     spin_lock_time += chrono::duration<double>(t2 - t1).count();

    //     t1 = chrono::high_resolution_clock::now();
    //     pthread_spin_unlock(&spinlock);
    //     t2 = chrono::high_resolution_clock::now();
    //     spin_unlock_time += chrono::duration<double>(t2 - t1).count();
    // }
    // берем среднее значение
    // spin_lock_time /= EXP_NUM;
    // spin_unlock_time /= EXP_NUM;


    // cout << "Мьютекс\n";
    // cout << "\tlock: " << mutex_lock_time << endl;
    // cout << "\tunlock: " << mutex_unlock_time << endl;
    // cout << "Спинлок\n";
    // cout << "\tlock: " << spin_lock_time << endl;
    // cout << "\tunlock: " << spin_unlock_time << endl;