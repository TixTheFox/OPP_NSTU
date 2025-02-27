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
    // приводим указатель void* к типу pthread_attr_t
    pthread_attr_t thread_attr = *(pthread_attr_t *) arg;

    // получаем атрибуты потока
    int detach_state;
    size_t guard_size;
    void* stack_addr;
    size_t stack_size;

    int err;
    err = pthread_attr_getdetachstate(&thread_attr, &detach_state);
    if (err != 0) {
        cout << "Невозможно получить состояние потока: " << strerror(err) << endl;
        pthread_exit(NULL);
    }

    err = pthread_attr_getguardsize(&thread_attr, &guard_size);
    if (err != 0) {
        cout << "Невозможно получить размер охранной зоны потока: " << strerror(err) << endl;
        pthread_exit(NULL);
    }

    err = pthread_attr_getstack(&thread_attr, &stack_addr, &stack_size);
    if (err != 0) {
        cout << "Невозможно получить информацию о стеке потока: " << strerror(err) << endl;
        pthread_exit(NULL);
    }

    cout << "Информация об атрибутах потока\n";
    cout << "Состояние потока: " << ((detach_state == PTHREAD_CREATE_DETACHED) ? "PTHREAD_CREATE_DETACHED" : "PTHREAD_CREATE_JOINABLE") << endl;
    cout << "Размер охранной зоны (в байтах): " << guard_size << endl;
    cout << "Адрес стека: " << stack_addr << endl;
    cout << "Размер стека (в байтах): " << stack_size << endl;
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
    err =  pthread_attr_setstacksize(&thread_attr, 1*1024*1024); 
    if(err != 0) { 
        cout << "Невозможно задать размер стека: " << strerror(err) << endl; 
        exit(-1); 
    }


    for (int i = 0; i < thread_num; i++) {    
        // Создаём поток c передачей аргумента
        err =  pthread_create(&thread, &thread_attr, thread_job, (void *)&thread_attr); 
        // Если при создании потока произошла ошибка, выводим  
        // сообщение об ошибке и прекращаем работу программы 
        if(err != 0) { 
            cout << "Cannot create a thread: " << strerror(err) << endl; 
            exit(-1); 
        } 
    }  

    // Ожидаем завершения созданного потока перед завершением работы программы 
    pthread_exit(NULL); 
    
    // Освобождаем память, занятую под хранение атрибутов потока 
    pthread_attr_destroy(&thread_attr);
}