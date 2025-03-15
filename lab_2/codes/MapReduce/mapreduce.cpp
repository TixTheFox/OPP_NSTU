#include <iostream>
#include <vector>
#include <map>
#include <pthread.h>
#include <list>
#include <chrono>

using namespace std;

// Размер решаемой задачи (массива целых чисел)
#define ARR_SIZE (long int)1e6
// Число проводимых экспериментов
#define EXP_NUM 20

pthread_mutex_t mapMutex;       // мьютекс, используемый в mapThread
pthread_mutex_t reduceMutex;    // мьютекс, используемый в reduceThread

// Структура-аргумент вызова потока mapThread
struct MapThreadInfo {
    int* array;                     // ссылка на обрабатывемый массив
    int start;                      // Индекс первого обрабатываемого в потоке элемента массива
    int end;                        // Индекс элемента, следующего до последнего обрабатываемого элемента

    int (*mapFunc)(int);            // Функция map, применяемая к каждому элементу заданной части массива

    map<int, vector<int>>* result;  // ссылка на map, хранящий результат работы map (соответствие "число i" - "массив значений mapFunc(i)")
};

// Структура-аргумент вызова потока reduceThread
struct ReduceThreadData {
    map<int, vector<int>>* map_unite;   // ссылка на объединенный словарь map (слияние результатов mapThread)
    list<int> keys;                     // список ключей, который обрабатывает текущий поток

    int (*reduceFunc)(vector<int>);     // выполняемая reduce-функция 

    map<int, int>* result;              // ссылка на map, хранящий результат работы всех reduce потоков
};

// Функция Map, выполняемая в потоках
void* mapThread(void* arg) {
    MapThreadInfo data = *(MapThreadInfo*)arg;

    map<int, vector<int>> local_result; // хранит локальный результат работы mapFunc
    
    for (int i = data.start; i != data.end; i++) {
        int elem = data.array[i];
        local_result[elem].push_back(data.mapFunc(elem)); // добавляем результат работы mapFunc в local_result
    }

    // захватываем доступ для работы с общим словарем результатов data.result
    pthread_mutex_lock(&mapMutex);
    // записываем в него всё, что лежит в local_result
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

    map<int, int> local_result; // хранит локальный результат работы reduceFunc

    // выполняем reduceFunc для каждого элемента data.map_unite
    for (int key : data.keys) {
        vector<int>& map_unite_elem = (*data.map_unite).at(key);
        int elem = data.reduceFunc(map_unite_elem);
        local_result[key] = elem;        
    }
    
    // захватываем доступ к словарю data.result
    pthread_mutex_lock(&reduceMutex);
    // записываем все, что лежит в local_result
    for (auto elem : local_result) {
        (*data.result)[elem.first] = elem.second;
    }
    pthread_mutex_unlock(&reduceMutex);

    pthread_exit(NULL);
}

// Основная функция MapReduce
void MapReduce(int arr[], int (*mapFunc)(int), int (*reduceFunc)(vector<int>), int maxThreads) {
    // массив аргументов вызова mapThread
    MapThreadInfo* map_thread_args = new MapThreadInfo[maxThreads];
    
    // хранит результат работы всех потоков mapThread
    map<int, vector<int>> map_result;

    // разбиваем задачу arr между потоками
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

    // -- создаем потоки map
    pthread_t* threads =  new pthread_t[maxThreads];

    for (int i = 0; i < maxThreads; ++i) {
        pthread_create(&threads[i], NULL, mapThread, (void*)&map_thread_args[i]);
    }

    for (int i = 0; i < maxThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
    // --

    // объединяем результаты Map
    map<int, vector<int>> map_unite;
    for (const auto& kv : map_result) {
        map_unite[kv.first].insert(
            map_unite[kv.first].end(), kv.second.begin(), kv.second.end()
        );
    }

    // - формируем аргументы Reduce
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
    // -

    // создаем потоки reduce
    for (int i = 0; i < maxThreads; i++) {
        pthread_create(&threads[i], NULL, reduceThread, &reduce_thread_args[i]);
    }

    for (int i = 0; i < maxThreads; i++) {
        pthread_join(threads[i], NULL);
    }


    // (закомментировано, т.к. сейчас программа нацелена на измерение времени, а не вывод результата)
    // Выводим результаты 
    // for (const auto& data : reduce_result) {
    //     cout << data.first << ": " << data.second<< endl;
    // }

    delete map_thread_args, threads;
}

// Определяем mapFunc
// Будет подсчитывать число четных чисел
int myMapFunc(int arg) {
    return arg % 2 == 0 ? 1 : 0;
}

// определяем reduceFunc
int myReduceFunc(vector<int> data) {
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

    // создаем случайный массив для работы 
    int* data = new int[ARR_SIZE];
    srand(time(0));
    for (int i = 0; i < ARR_SIZE; i++) {
        data[i] = 1 + rand() % 10;
    }

    
    double time;
    for (int i = 0; i < EXP_NUM; i++) {
        auto t1 = chrono::high_resolution_clock::now();
        MapReduce(data, myMapFunc, myReduceFunc, maxThreads);
        auto t2 = chrono::high_resolution_clock::now();
        time += chrono::duration<double>(t2 - t1).count();
    }

    cout << "Среднее время работы программы " << time/EXP_NUM << endl;

    delete data;

    return 0;
}