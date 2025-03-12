#include <cstdlib>
#include <cstring>
#include <iostream>

#include "condvar.h"

using namespace std;

#define err_exit(code, str)               \
  {                                       \
    cerr << str << ": " << strerror(code) \
         << endl;                         \
    exit(EXIT_FAILURE);                   \
  }

enum store_state
{
  EMPTY,
  FULL
} state = EMPTY;

pthread_mutex_t mutex;
MyCondvar cond;

int store;

void *producer(void *arg)
{
  int err;
  //
  while (true)
  {
    // Захватываем мьютекс и ожидаем освобождения склада
    err = pthread_mutex_lock(&mutex);
    if (err != 0)
      err_exit(err, "Cannot lock mutex");
    while (state == FULL)
    {
      cond.wait(&mutex);
    }
    // Получен сигнал, что на складе не осталось товаров.
    // Производим новый товар.
    store = rand();
    state = FULL;
    // Посылаем сигнал, что на складе появился товар.
    cond.signal();
    err = pthread_mutex_unlock(&mutex);
  }
}

void *consumer(void *arg)
{
    int err;
    while (true) {
        // Захватываем мьютекс и ожидаем появления товаров на складе
        err = pthread_mutex_lock(&mutex);
        if (err != 0)
          err_exit(err, "Cannot lock mutex");
        while (state == EMPTY)
        {
          cond.wait(&mutex);
        }
        // Получен сигнал, что на складе имеется товар.
        // Потребляем его.
        cout << "Consuming number " << store << "...";
        sleep(1);
        cout << "done" << endl;
        state = EMPTY;
        // Посылаем сигнал, что на складе не осталось товаров.
        cond.signal();
        err = pthread_mutex_unlock(&mutex);
        if (err != 0)
          err_exit(err, "Cannot unlock mutex");
    }
}

int main() {
    int err;
    //
    pthread_t thread1, thread2; // Идентификаторы потоков
                                // Инициализируем мьютекс и условную переменную
    err = pthread_mutex_init(&mutex, NULL);
    if (err != 0)
      err_exit(err, "Cannot initialize mutex");
    // Создаём потоки
    err = pthread_create(&thread1, NULL, producer, NULL);
    if (err != 0)
      err_exit(err, "Cannot create thread 1");
    err = pthread_create(&thread2, NULL, consumer, NULL);
    if (err != 0)
      err_exit(err, "Cannot create thread 2");
    // Дожидаемся завершения потоков
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // Освобождаем ресурсы, связанные с мьютексом
    pthread_mutex_destroy(&mutex);
}