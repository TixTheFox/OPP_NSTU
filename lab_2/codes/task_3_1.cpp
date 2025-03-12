#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
#include <chrono>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }    

#define EXP_NUM 50 // число экспериментов (берется среднее время за число экспериментов)

// объекты синхронизации
pthread_mutex_t mutex;
pthread_spinlock_t spinlock;

// объект, над которым будем производить оперции
int counter = 0;

// функция потока, использующая мьютекс для блокировок
void* thread_mutex(void* arg) {
    int err;

    err = pthread_mutex_lock(&mutex);
    if (err != 0) err_exit(err, "Невозможно захватить мьютекс")
    
    for(int i = 0; i < 1000; i++) counter++;
    
    err = pthread_mutex_unlock(&mutex);
    if (err != 0) err_exit(err, "Невозможно освободить мьютекс")

    pthread_exit(NULL);
}

// функция потока, использующая спинлок для блокировок
void* thread_spinlock(void* arg) {
    int err;

    err = pthread_spin_lock(&spinlock);
    if (err != 0) err_exit(err, "Невозможно захватить спинлок")

    for(int i = 0; i < 1000; i++) counter++;

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

    cout << "Mutex time: " << mutex_time << endl;
    cout << "Spinlock time: " << spin_time << endl;

    // очистка памяти
    pthread_mutex_destroy(&mutex);
    pthread_spin_destroy(&spinlock);
    delete threads;
}