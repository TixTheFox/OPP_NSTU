#include <omp.h> 
#include <iostream> 
#include <chrono>

using namespace std; 

#define EXP_NUM 50

int main(int argc, char* argv[]) {  
    // проверки введенного аргумента
    if (argc != 2) {
        cout << "Недопустимое число аргументов. Введите один аргумент - число потоков, которые будут запущены для теста\n";
        exit(-1);
    }
    
    int thread_count = 0;

    if ( ((thread_count = atoi(argv[1])) == 0) || thread_count <= 0) {
        cout << "Число потоков должно быть целым положительным числом\n";
        exit(-1);
    }

    // устанавливаем число создаваемых потоков
    omp_set_num_threads(thread_count);

    const int a_size = 1e5; 
    int a[a_size], id, size; 

    for(int i = 0; i<100; i++) 
        a[i] = i; 

    double time_result = 0;
    for (int i = 0; i < EXP_NUM; i++) {
        int sum = 0; 
        auto t1 = chrono::high_resolution_clock::now(); 
        #pragma omp parallel private(size, id) shared(sum)
        { 
            id = omp_get_thread_num(); 
            size = omp_get_num_threads(); 
            // Разделяем работу между потоками 
            int integer_part = a_size / size; 
            int remainder = a_size % size; 
            int a_local_size = integer_part + ((id < remainder) ? 1 : 0); 
            int start = integer_part * id + ((id < remainder) ? id : remainder); 
            int end = start + a_local_size; 
            // Каждый поток суммирует элементы  
            // своей части массива 
            #pragma omp critical
            {
                for(int i = start; i < end; i++) 
                    sum += a[i]; 
            }
        } 
        auto t2 = chrono::high_resolution_clock::now();
        time_result += chrono::duration<double>(t2 - t1).count();
    }

    cout << "Среднее время выполения: " << time_result / EXP_NUM << endl;
    return 0; 
} 