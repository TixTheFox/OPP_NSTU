#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
#include <chrono>

using namespace std; 
 
// функция потока
void *thread_job(void *arg) { 
    pthread_exit(NULL);
}

int main() { 
    // переменная потока
    pthread_t thread; 
    // переменная с кодом ошибки
    int err;
    
    // число экспериментов
    int counts = 100;
    // переменная для хранения результатов измерения
    double time_result = 0;

    for (int i = 0; i < counts; i++) {
        // t1, t2 - хранят информацию о моменте времени now()
        auto t1 = std::chrono::high_resolution_clock::now();
        err =  pthread_create(&thread, NULL, thread_job, NULL);
        auto t2 = std::chrono::high_resolution_clock::now();
        time_result += std::chrono::duration<double>(t2 - t1).count();
        
        // выводим сообщение об ошибке, в случае ее наличия,
        // и выходим из программы
        if(err != 0) { 
            cout << "Cannot create thread: " << strerror(err) << endl; 
            exit(-1); 
        }
    }

    // выводим среднее время 
    cout << "avg thread init time: " << time_result / counts << "\n";


    // число операций, выполняемых за один эксперимент
    int operations_number = 30000;
    // переменная, над которой будут производиться 
    // вычисления
    int tmp = 1;
    time_result = 0;

    for (int i = 0; i < counts; i++) {
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < operations_number; j++) {
            tmp *= 2;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_result += std::chrono::duration<double>(t2 - t1).count();
    }

    cout << "number of operations: " << operations_number << endl;
    cout << "avg operations execution time: " << time_result / counts << "\n";
    pthread_exit(NULL); 
} 