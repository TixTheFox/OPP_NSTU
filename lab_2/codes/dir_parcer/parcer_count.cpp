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

char* search_str;

queue<fs::path> tasks;
pthread_mutex_t task_queue_mutex;
pthread_spinlock_t console_out_spin, active_thread_spin;
pthread_cond_t task_queue_condvar;
int active_threads = 0;
bool stop_work = 0;


fs::path getTask() {
    pthread_mutex_lock(&task_queue_mutex);
    while (tasks.empty() && !stop_work) {
        pthread_cond_wait(&task_queue_condvar, &task_queue_mutex);
    }

    if (stop_work) {
        pthread_mutex_unlock(&task_queue_mutex);
        return {};
    }

    fs::path task = tasks.front();
    tasks.pop();
    pthread_mutex_unlock(&task_queue_mutex);
    pthread_spin_lock(&active_thread_spin);
    active_threads++;
    pthread_spin_unlock(&active_thread_spin);
    return task;
}


int fileCountSubstring(fs::path filename) {
    string tmp;
    int res = 0;
    ifstream f;
    f.open(filename);
    if (!f){
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
    char* task;
    error_code err;
    while(true) {
        fs::path task = getTask();
        if (task.empty()) {
            break;
        } else if (fs::is_directory(task)) {
            pthread_mutex_lock(&task_queue_mutex);
            for (const auto& entry : fs::directory_iterator(task, err)) {
                tasks.push(entry.path());
                pthread_cond_signal(&task_queue_condvar);
            }
            pthread_mutex_unlock(&task_queue_mutex);
        } else {
            int substr_count = fileCountSubstring(task);
            pthread_spin_lock(&console_out_spin);
            cout << task << " : " << substr_count << " times\n";
            pthread_spin_unlock(&console_out_spin);
        }

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