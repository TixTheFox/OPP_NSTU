#ifndef CONDVAR_PP
#define CONDVAR_PP

#include <pthread.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }   

#define SLEEP_TIMEOUT_MS 1e-5

/* 
    Класс реализует условную переменную через цикл ожидания
    К переменной может обращаться множество потоков - они будут записаны в очередь.
*/
class MyCondvar {
  public:
    MyCondvar();
    ~MyCondvar();
    // поставить вызывающий поток на ожидание сигнала
    // mutex - внешний мьютекс, связанный с condvar (как и pthread_cond_t)
    void wait(pthread_mutex_t *mutex);
    // послать сигнал одному ожидающему потоку
    void signal();
    
  private:
    // Элемент очереди потоков
    struct lock_queue_elem {
        bool* val;              // логическая переменная, управляющая циклом ожидания потока. Если false, считается, что поток получил сигнал
        lock_queue_elem* next;  // ссылка на след. элемент очереди
    };

    // Очередь ожидающих потоков
    struct lock_queue {
        lock_queue_elem* head; // первый элемент
        lock_queue_elem* last; // последний элемент

        // инициализация структуры
        void init() {
            head = NULL;
            last = NULL;
        }

        // уничтожение структуры
        void destroy() {
            while(!is_empty()) {
                pop();
            }
        }

        // добавить элемент в очередь
        void push(bool* lock) {
            if (head == NULL) {
                head = new lock_queue_elem;
                head->val = lock;
                head->next = NULL;
                last = head;
            } else {
                lock_queue_elem* new_elem = new lock_queue_elem;
                last->next = new_elem;
                last = new_elem;
                new_elem->val = lock;
                new_elem->next = NULL;
            }
        }

        // вытащить элемент из очереди
        bool* pop() {
            bool* retval;
            if (is_empty()) {
                retval = NULL;
            } else if (head == last) {
                lock_queue_elem* popped = head;
                head = NULL;
                last = NULL;
                retval = popped->val;
                delete popped;
            } else {
                retval = head->val;
                lock_queue_elem* popped = head;
                head = popped->next;
                delete popped;
            }
            return retval;
        }

        // проверка на пустота
        bool is_empty() {
            return head == NULL ? true : false;
        }
    };

    pthread_mutex_t condvar_mutex;  // мьютекс для работы с условной переменной (обеспечить "аккуратное" взаимодействие потоков с condvar)
    lock_queue locks_;              // очередь ожидающих потоков
};

#endif