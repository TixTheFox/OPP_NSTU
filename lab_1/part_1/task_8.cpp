#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
#include <chrono>
#include <string>
#include <cmath>
#include <iterator>
#include <algorithm>

using namespace std;

// число проводимых экспериментов
#define EXP_NUM 100

// структура для передачи информации о массиве для 
// обработки в потоке
struct ArrayArgInfo {
    double* array;  // ссылка на массив
    int start;      // Индекс первого обрабатываемого в потоке элемента массива
    int end;        // Индекс элемента, следующего до последнего обрабатываемого элемента
};

// функция копирования массива из src в dest. size - размер копируемого массива
void copyArray(double* src, double* dest, int size) {
    for (int i = 0; i < size; i++) dest[i] = src[i];
}

// функция добавления единицы
void *addOneFunc(void* arg) {
    ArrayArgInfo* array_args = (ArrayArgInfo *) arg;
    for (int i = array_args->start; i < array_args->end; i++) {
        array_args->array[i]++;
    }
}

// функция возведения элемента в квадрат
void *squareFunc(void* arg) {
    ArrayArgInfo* array_args = (ArrayArgInfo *) arg;
    for (int i = array_args->start; i < array_args->end; i++) {
        array_args->array[i] *= array_args->array[i];
    }
}

// функция взятия синуса
void *sinFunc(void* arg) {
    ArrayArgInfo* array_args = (ArrayArgInfo *) arg;
    for (int i = array_args->start; i < array_args->end; i++) {
        array_args->array[i] = sin(array_args->array[i]);
    }
}

// функция, обрабатывающая ввод пользователя
// в качестве параметров принимает ссылки на переменные, которые будут содержать
// введенную пользователем информацию
void input(int& array_size, int& operation, int& threads_count) {
    // ввод размера массива
    while (!array_size) {
        cout << "Введите размер массива: ";
        cin >> array_size;  
        if (cin.fail() || array_size <= 0) {
            cout << "Невозможный размер массива, повторите ввод\n";
            array_size = 0;
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    // ввод операции
    cout << "Доступные операции:\n" 
         << "1 - прибавить к каждому элементу единицу\n" 
         << "2 - возвести элемент в квадрат\n" 
         << "3 - взять синус каждого элемента массива\n";
    
    while (!operation) {
        cout << "Введите номер операции: ";
        cin >> operation;  
        if (cin.fail() || operation < 1 || operation > 3) {
            cout << "Неизвестный номер операции, повторите ввод\n";
            operation = 0;
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    //ввод кол-ва потоков
    while (!threads_count) {
        cout << "Введите количество потоков: ";
        cin >> threads_count;  
        if (cin.fail() || threads_count <= 0) {
            cout << "Невозможное число потоков, повторите ввод\n";
            threads_count = 0;
            cin.clear();
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}

// возвращает среднее время однопоточной обработки массива
double noThreadOperationTime(double* array, int array_size, int operation) {
    double time_result = 0;
    for (int j = 0; j < EXP_NUM; j++) {
        auto t1 = std::chrono::high_resolution_clock::now();
        switch(operation) {
            case 1:
                for (int i = 0; i < array_size; i++) {
                    array[i]++;
                }
                break;
            case 2:
                for (int i = 0; i < array_size; i++) {
                    array[i] *= array[i];
                }
                break;
            case 3:
                for (int i = 0; i < array_size; i++) {
                    array[i] = sin(array[i]);
                }
                break;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_result += std::chrono::duration<double>(t2 - t1).count();
    }

    return time_result / EXP_NUM;
}

// возвращает среднее время многопоточной обработки массива
double threadsOperationTime(double* array, int array_size, int operation, int threads_count) {
    double time_result = 0;

    void* (*thread_func) (void* arg);
    switch(operation) {
        case 1:
            thread_func = addOneFunc;
            break;
        case 2:
            thread_func = squareFunc;
            break;
        case 3:
            thread_func = sinFunc;
            break;    
    }

    pthread_t* threads = new pthread_t[threads_count];
    ArrayArgInfo* thread_args = new ArrayArgInfo[threads_count]; 

    // этот блок равномерно разбивает массив между потоками 
    // информация о разбиении хранится в структурах ArrayArgInfo
    int part_size = array_size / threads_count;
    int part_remain = array_size % threads_count;
    int start = 0;
    for (int i = 0; i < threads_count; i++) {
        int end = start + part_size + (i < part_remain ? 1 : 0);
        thread_args[i].array = array;
        thread_args[i].start = start;
        thread_args[i].end = end;
        start = end;
    }

    int err = 0;

    for (int j = 0; j < EXP_NUM; j++) {
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < threads_count; i++) {
            err = pthread_create(&threads[i], NULL, thread_func, (void *) &thread_args[i]);
            if (err != 0) {
                cout << "Невозможно создать поток: " << strerror(err) << "\n";
                exit(-1);
            }
        }

        for (int i = 0; i < threads_count; i++) {
            pthread_join(threads[i], NULL);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_result += std::chrono::duration<double>(t2 - t1).count();
    }


    delete threads;
    delete thread_args;
    return time_result / EXP_NUM;
}

int main() {
    // хранит размер массива
    int array_size = 0;
    // хранит операцию, выбранную пользователем
    int operation = 0;
    // хранит число создаваемых потоков
    int threads_count = 0; 
    // хранит код ошибки
    int err = 0;

    // ввод параметров
    input(array_size, operation, threads_count);
    
    // создание и инициализация массива
    double* array = new double[array_size];
    for (int i = 0; i < array_size; i++) {
        array[i] = rand() % 100;
    }

    double* working_copy = new double[array_size]; 
    copyArray(array, working_copy, array_size);

    double no_thread_time = noThreadOperationTime(working_copy, array_size, operation);

    copyArray(array, working_copy, array_size);

    double threads_time = threadsOperationTime(working_copy, array_size, operation, threads_count);

    cout << "Время однопоточного выполнения: " << no_thread_time << "\n";
    cout << "Время многопоточного выполнения: " << threads_time << "\n";

    delete array;
    delete working_copy;
}