#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
 
using namespace std; 
 
/* Функция, которую будет исполнять созданный поток */ 
void *thread_job(void *arg) 
{ 
    cout << "Thread is running..." << endl; 
} 
 
int main(int argc, char *argv[]) 
{ 
    // получаем число потоков из параметров запуска программы
    if (argc != 2) {
        cout << "Некорректное число параметров запуска программы, разрешен 1, обнаружено: " << (argc - 1) << endl;;
        exit(-1);
    }

    int thread_num = 0;
    if ( (thread_num= atoi(argv[1])) == 0 || thread_num <= 0) {
        cout << "Число потоков должно быть целым положительным. Текущее значение: " << thread_num << endl;
        exit(-1);
    }

    // Определяем переменные: идентификатор потока и код ошибки 
    pthread_t thread; 
    int err;  
    for (int i = 0; i < thread_num; i++) {    
        // Создаём поток
        err =  pthread_create(&thread, NULL, thread_job, NULL); 
        // Если при создании потока произошла ошибка, выводим  
        // сообщение об ошибке и прекращаем работу программы 
        if(err != 0) { 
            cout << "Cannot create a thread: " << strerror(err) << endl; 
            exit(-1); 
        } 
    }  
    // Ожидаем завершения созданного потока перед завершением  
    // работы программы 
    pthread_exit(NULL); 
}