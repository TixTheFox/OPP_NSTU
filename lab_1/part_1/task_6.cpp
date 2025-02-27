#include <cstdlib> 
#include <iostream> 
#include <cstring> 
#include <pthread.h> 
 
using namespace std; 
 
struct thread_args {
    int num;
    char str[64];
};

/* Функция, которую будет исполнять созданный поток */ 
void *thread_job(void *arg) 
{ 
    // приводим void * к пользовательской структуре
    thread_args thr_args = *(thread_args*) arg;
    cout << "Получены аргументы " << thr_args.num << " " << thr_args.str << endl; 
} 
 
int main(int argc, char *argv[]) 
{ 
    // --- получаем число потоков из параметров запуска программы
    if (argc != 2) {
        cout << "Некорректное число параметров запуска программы, разрешен 1, обнаружено: " << (argc - 1) << endl;;
        exit(-1);
    }

    int thread_num = 0;
    if ( (thread_num= atoi(argv[1])) == 0 || thread_num <= 0) {
        cout << "Число потоков должно быть целым положительным. Текущее значение: " << thread_num << endl;
        exit(-1);
    }

    // --- Определяем переменные: идентификатор потока, атрибуты потока и код ошибки 
    pthread_t thread; 
    pthread_attr_t thread_attr;
    int err; 

    // --- инициализируем структуру thread_attr
    err =  pthread_attr_init(&thread_attr); 
    if(err != 0) { 
        cout << "Невозможно инициализировать атрибуты потока: " << strerror(err) << endl; 
        exit(-1); 
    }

    // задаем размер стека
    err =  pthread_attr_setstacksize(&thread_attr, 5*1024*1024); 
    if(err != 0) { 
        cout << "Невозможно задать размер стека: " << strerror(err) << endl; 
        exit(-1); 
    }

    // --- Инициализируем структуру, которую будем передавать в поток
    thread_args thr_args = {10, "Hello, World!"};


    for (int i = 0; i < thread_num; i++) {    
        // Создаём поток c передачей аргумента
        err =  pthread_create(&thread, &thread_attr, thread_job, (void *)&thr_args); 
        // Если при создании потока произошла ошибка, выводим  
        // сообщение об ошибке и прекращаем работу программы 
        if(err != 0) { 
            cout << "Cannot create a thread: " << strerror(err) << endl; 
            exit(-1); 
        } 
    }  

    // Освобождаем память, занятую под хранение атрибутов потока 
    pthread_attr_destroy(&thread_attr);

    // Ожидаем завершения созданного потока перед завершением работы программы 
    pthread_exit(NULL); 
}