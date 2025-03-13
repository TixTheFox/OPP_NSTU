#include <pthread.h>
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <queue>
#include <filesystem>
#include <string>
#include <fstream>

using namespace std;
namespace fs = filesystem;

#define THREAD_COUNT 4

char* search_str;

queue<fs::path> tasks;
pthread_mutex_t task_queue_mutex, active_thread_mutex;
pthread_spinlock_t console_out_spin;
pthread_cond_t task_queue_condvar;
int active_threads = 0;
bool stop_work = 0;

bool isTxtFile(string filename) {
    if (filename.length() < 4)
        return false;
    return 0 == filename.compare(filename.length() - 4, 4, ".txt");
}

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
    active_threads++;
    pthread_mutex_unlock(&task_queue_mutex);
    return task;
}

bool fileContainsSubstring(fs::path filename) {
    string tmp;
    bool res = false;
    ifstream f;
    f.open(filename);
    if (!f){
        pthread_spin_lock(&console_out_spin);
        cout << "Невозможно открыть файл: " << filename << "\n";
        pthread_spin_unlock(&console_out_spin);
        res = false;
    } else {
        while(getline(f, tmp) && res == false) {
            if (tmp.find(search_str) != string::npos) {
                res = true;
            }
        }
        f.close();
    }
    return res;
}

void* working_thread(void*) {
    char* task;
    while(true) {
        fs::path task = getTask();
        if (task.empty()) {
            break;
        } else if (fs::is_directory(task)) {
            pthread_mutex_lock(&task_queue_mutex);
            for (const auto& entry : fs::directory_iterator(task)) {
                tasks.push(entry.path());
            }
            pthread_mutex_unlock(&task_queue_mutex);
            pthread_cond_signal(&task_queue_condvar);
        } else if (isTxtFile(task)) {
            if(fileContainsSubstring(task)) {
                pthread_spin_lock(&console_out_spin);
                cout << task << "\n";
                pthread_spin_unlock(&console_out_spin);
            }
        }

        pthread_mutex_lock(&active_thread_mutex);
        active_threads--;
        if (tasks.empty() && active_threads == 0) {
            stop_work = 1;
            pthread_cond_broadcast(&task_queue_condvar);
        }
        pthread_mutex_unlock(&active_thread_mutex);
    }
    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Ожидается аргументов: 2, получено: " << argc - 1 << "\n";
        exit(-1);   
    }

    if (!fs::exists(argv[1]) || !fs::is_directory(argv[1])) {
        cout << "Не найдено директории " << argv[1] << "\n";
        exit(-1);
    }

    pthread_mutex_init(&task_queue_mutex, NULL);
    pthread_mutex_init(&active_thread_mutex, NULL);
    pthread_cond_init(&task_queue_condvar, NULL);
    pthread_spin_init(&console_out_spin, PTHREAD_PROCESS_PRIVATE);

    tasks.push(argv[1]);
    search_str = argv[2];

    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, working_thread, NULL);
    }
    
    for (int i = 0; i < THREAD_COUNT; i++) {
       pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&task_queue_mutex);
    pthread_mutex_destroy(&active_thread_mutex);
    pthread_cond_destroy(&task_queue_condvar);
    pthread_spin_destroy(&console_out_spin);
    
    return 0;
}