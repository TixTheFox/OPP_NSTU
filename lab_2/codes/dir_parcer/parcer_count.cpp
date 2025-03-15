#include <pthread.h>
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <queue>
#include <filesystem>
#include <string>
#include <fstream>
#include <unistd.h>
#include <chrono>

using namespace std;
namespace fs = filesystem;

char* search_str;   // строка, которую будем искать в файлах

queue<fs::path> tasks;                  // очередь задач (имена файлов)
pthread_mutex_t task_queue_mutex;       // мьютекс для взаимодействия с этой очередью
pthread_cond_t task_queue_condvar;      // условная переменная для работы с очередью
pthread_spinlock_t console_out_spin;    // спинлок для захвата вывода на экран
pthread_spinlock_t active_thread_spin;  // спинлок для работы с счетчиком "активных" потоков
int active_threads = 0;                 // счетчик "активных" потоков
bool stop_work = false;                 // флаг завершения работы


/*
    Функция для взятия задачи потоком из очереди
*/
fs::path getTask() {
    // захватываем доступ к очереди
    pthread_mutex_lock(&task_queue_mutex);
    // если очередь пуста и при это работа не завершена, 
    // то встаем в ожидание сигнала условной переменной
    while (tasks.empty() && !stop_work) {
        pthread_cond_wait(&task_queue_condvar, &task_queue_mutex);
    }

    // если, пока мы ждали, работа завершилась, то и мы ничего не делаем с очередью
    if (stop_work) {
        pthread_mutex_unlock(&task_queue_mutex);
        return {};
    }

    // берем задачу
    fs::path task = tasks.front();
    tasks.pop();
    // возвращаем доступ к очереди
    pthread_mutex_unlock(&task_queue_mutex);
    // безопасно увеличиваем число "активных" потоков
    pthread_spin_lock(&active_thread_spin);
    active_threads++;
    pthread_spin_unlock(&active_thread_spin);
    return task;
}


/*
    Функция для подсчета вхождений подстроки в файле
    Возвращает число вхождений строки в файле filename
*/
int fileCountSubstring(fs::path filename) {
    string tmp;
    int res = 0;
    ifstream f;
    f.open(filename);
    if (!f){
        // безопасно выводим в консоль сообщение о невозможности открытия файла
        pthread_spin_lock(&console_out_spin);
        cout << "Невозможно открыть файл: " << filename << "\n";
        pthread_spin_unlock(&console_out_spin);
        res = false;
    } else {
        while(getline(f, tmp)) {
            if (tmp.find(search_str) != string::npos) {
                res++;
            }
        }
        f.close();
    }
    return res;
}


void* working_thread(void*) {
    char* task;         // текущая задача потока
    error_code err;     // возникающая ошибка
    while(true) {
        fs::path task = getTask();
        if (task.empty()) {
            break;
        } else if (fs::is_directory(task)) {
            // если текущий файл - директория, то захватываем доступ к очереди задач
            // и наполняем ее файлами из этой директории
            pthread_mutex_lock(&task_queue_mutex);
            for (const auto& entry : fs::directory_iterator(task, err)) {
                tasks.push(entry.path());
                
                // посылаем сигналы ожидающим потокам (если такие есть), 
                // что появились задачи 
                pthread_cond_signal(&task_queue_condvar);
            }
            pthread_mutex_unlock(&task_queue_mutex);
        } else {
            // иначе если файл - не директория, то считаем подстроки
            int substr_count = fileCountSubstring(task);
            // безопасно выводим результат работы
            pthread_spin_lock(&console_out_spin);
            cout << task << " : " << substr_count << " times\n";
            pthread_spin_unlock(&console_out_spin);
        }

        
        // после выполнения работы над текущей задачей:
        //  - уменьшаем число активных потоков
        //  - если при этом задач и других активных потоков нет, то фиксируем 
        //    конец работы программы (stop_work = 1)
        pthread_spin_lock(&active_thread_spin);
        active_threads--;
        if (tasks.empty() && active_threads == 0) {
            stop_work = 1;
            pthread_cond_broadcast(&task_queue_condvar);
        }
        pthread_spin_unlock(&active_thread_spin);
    }
    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Ожидается аргументов: 3, получено: " << argc - 1 << "\n";
        exit(-1);   
    }

    if (!fs::exists(argv[1]) || !fs::is_directory(argv[1])) {
        cout << "Не найдено директории " << argv[1] << "\n";
        exit(-1);
    }

    int maxThreads;
    if ( ((maxThreads = atoi(argv[3])) == 0) || maxThreads <= 0) {
        cout << "Число потоков должно быть целым положительным числом\n";
        exit(-1);
    }

    pthread_mutex_init(&task_queue_mutex, NULL);
    pthread_cond_init(&task_queue_condvar, NULL);
    pthread_spin_init(&console_out_spin, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&active_thread_spin, PTHREAD_PROCESS_PRIVATE);

    auto t1 = chrono::high_resolution_clock::now();

    tasks.push(argv[1]);
    search_str = argv[2];

    pthread_t* threads = new pthread_t[maxThreads];
    for (int i = 0; i < maxThreads; i++) {
        pthread_create(&threads[i], NULL, working_thread, NULL);
    }

    for (int i = 0; i < maxThreads; i++) {
       pthread_join(threads[i], NULL);
    }

    auto t2 = chrono::high_resolution_clock::now();
    double time_result =  chrono::duration<double>(t2 - t1).count();

    cout << "Время поиска: " << time_result << endl;

    pthread_mutex_destroy(&task_queue_mutex);
    pthread_cond_destroy(&task_queue_condvar);
    pthread_spin_destroy(&console_out_spin);
    pthread_spin_destroy(&active_thread_spin);
    
    delete threads;

    return 0;
}