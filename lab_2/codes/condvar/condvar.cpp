#include "condvar.h"

MyCondvar::MyCondvar() {
    locks_.init();
}

MyCondvar::~MyCondvar() {
    locks_.destroy();
}

void MyCondvar::wait(pthread_mutex_t *mutex) {
    int err;    

    // Захватываем доступ к condvar
    err = pthread_mutex_lock(&condvar_mutex);
    if (err != 0)
        err_exit(err, "Невозможно захватить мьютекс (condvar)");
    
    // Встаем в очередь потоков
    bool* lock = new bool;
    *lock = true;
    locks_.push(lock);

    // возвращаем доступ к очереди
    err = pthread_mutex_unlock(&condvar_mutex);
    if (err != 0)
        err_exit(err, "Невозможно освободить мьютекс (condvar)");

    // открываем внешний мьютекс, связанный с condvar
    err = pthread_mutex_unlock(mutex);
    if (err != 0)
        err_exit(err, "Невозможно освободить мьютекс");

    // ждем сигнала
    while(*lock) {
        usleep(SLEEP_TIMEOUT_MS);
    }
    // удаляем ненужную память
    free(lock);

    // забираем внешний мьютекс
    err = pthread_mutex_lock(mutex);
    if (err != 0)
        err_exit(err, "Невозможно захватить мьютекс");
}

void MyCondvar::signal() {
    int err;
    // Захватываем доступ к condvar
    err = pthread_mutex_lock(&condvar_mutex);
    if (err != 0)
        err_exit(err, "Невозможно захватить мьютекс (condvar)");

    // вытаскиваем очередной элемент очереди и прерываем его цикл ожидания
    bool* lock = locks_.pop();
    if (lock != NULL) *lock = false;
    
    // возвращаем доступ к очереди
    err = pthread_mutex_unlock(&condvar_mutex);
    if (err != 0)
        err_exit(err, "Невозможно освободить мьютекс (condvar)");
}
