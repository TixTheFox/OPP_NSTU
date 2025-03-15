#include <iostream>
#include <vector>
#include <map>
#include <pthread.h>
#include <utility>
#include <list>
#include <chrono>

using namespace std;

#define ARR_SIZE (long int)1e6
#define EXP_NUM 20

pthread_mutex_t mapMutex;
pthread_mutex_t reduceMutex;


struct MapThreadInfo {
    int* array;     // ссылка на массив
    int start;      // Индекс первого обрабатываемого в потоке элемента массива
    int end;        // Индекс элемента, следующего до последнего обрабатываемого элемента

    int (*mapFunc)(int);

    map<int, vector<int>>* result;
};

struct ReduceThreadData {
    list<int> keys;
    map<int, vector<int>>* map_unite;

    int (*reduceFunc)(vector<int>);

    map<int, int>* result;
};

// Функция Map, выполняемая в потоках
void* mapThread(void* arg) {
    MapThreadInfo data = *(MapThreadInfo*)arg;

    map<int, vector<int>> local_result;
    
    for (int i = data.start; i != data.end; i++) {
        int elem = data.array[i];
        local_result[elem].push_back(data.mapFunc(elem));
    }

    pthread_mutex_lock(&mapMutex);
    for (auto item : local_result) {
        (*data.result)[item.first].insert(
            (*data.result)[item.first].end(), item.second.begin(), item.second.end()
        );
    }
    pthread_mutex_unlock(&mapMutex);

    pthread_exit(NULL);
}

// Функция Reduce, выполняемая в потоках
void* reduceThread(void* arg) {
    ReduceThreadData data = *(ReduceThreadData*) arg;

    map<int, int> local_result;

    for (int key : data.keys) {
        vector<int>& map_unite_elem = (*data.map_unite).at(key);
        int elem = data.reduceFunc(map_unite_elem);
        local_result[key] = elem;        
    }
    
    pthread_mutex_lock(&reduceMutex);
    for (auto elem : local_result) {
        (*data.result)[elem.first] = elem.second;
    }
    pthread_mutex_unlock(&reduceMutex);

    pthread_exit(NULL);
}

// Основная функция MapReduce
void MapReduce(int arr[], int (*mapFunc)(int), int (*reduceFunc)(vector<int>), int maxThreads) {

    MapThreadInfo* map_thread_args = new MapThreadInfo[maxThreads];

    map<int, vector<int>> map_result;

    int part_size = ARR_SIZE / maxThreads;
    int part_remain = ARR_SIZE % maxThreads;
    int start = 0;
    for (int i = 0; i < maxThreads; i++) {
        int end = start + part_size + (i < part_remain ? 1 : 0);
        map_thread_args[i].array = arr;
        map_thread_args[i].start = start;
        map_thread_args[i].end = end;
        map_thread_args[i].mapFunc = mapFunc;
        map_thread_args[i].result = &map_result;
        start = end;
    }

    pthread_t* threads =  new pthread_t[maxThreads];

    for (int i = 0; i < maxThreads; ++i) {
        pthread_create(&threads[i], NULL, mapThread, (void*)&map_thread_args[i]);
    }

    for (int i = 0; i < maxThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }


    // объединяем результаты Map
    map<int, vector<int>> map_unite;
    for (const auto& kv : map_result) {
        map_unite[kv.first].insert(
            map_unite[kv.first].end(), kv.second.begin(), kv.second.end()
        );
    }

    // формируем аргументы Reduce
    ReduceThreadData* reduce_thread_args = new ReduceThreadData[maxThreads];
    int tmp_count = 0;
    for (auto item : map_unite) {
        reduce_thread_args[tmp_count % maxThreads].keys.push_back(item.first);
        tmp_count++;
    }

    map<int, int> reduce_result;
    for (int i = 0; i < maxThreads; i++) {
        reduce_thread_args[i].map_unite = &map_unite; 
        reduce_thread_args[i].reduceFunc = reduceFunc;
        reduce_thread_args[i].result = &reduce_result;
    }

    for (int i = 0; i < maxThreads; i++) {
        pthread_create(&threads[i], NULL, reduceThread, &reduce_thread_args[i]);
    }

    for (int i = 0; i < maxThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Выводим результаты
    // for (const auto& data : reduce_result) {
    //     cout << data.first << ": " << data.second<< endl;
    // }

    delete map_thread_args, threads;
}

int mapFunc(int arg) {
    // return 1;
    return arg % 2 == 0 ? 1 : 0;
}

int reduceFunc(vector<int> data) {
    int sum = 0;
    if (!data.empty()) {
        for (int i : data) sum += i;
    }
    return sum;
}

int main(int argc, char* argv[]) {
    int maxThreads;
    if (argc != 2) {
        cout << "Ожидался 1 аргумент, получено " << argc - 1 << endl;
        exit(-1);
    } else if ( ((maxThreads = atoi(argv[1])) == 0) || maxThreads <= 0) {
        cout << "Число потоков должно быть целым положительным числом\n";
        return -1;
    }


    int* data = new int[ARR_SIZE];
    srand(time(0));
    for (int i = 0; i < ARR_SIZE; i++) {
        data[i] = 1 + rand() % 10;
    }

    
    double time;
    for (int i = 0; i < EXP_NUM; i++) {
        auto t1 = chrono::high_resolution_clock::now();
        MapReduce(data, mapFunc, reduceFunc, maxThreads);
        auto t2 = chrono::high_resolution_clock::now();
        time += chrono::duration<double>(t2 - t1).count();
    }

    cout << "Среднее время работы программы " << time/EXP_NUM << endl;

    delete data;

    return 0;
}