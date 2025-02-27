#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void *thread_func(void *arg) {
    sleep(100);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, 2 * 1024 * 1024);
    int count = 0;

    while (1) {
        if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
            std::cout << "max threads " << count << "\n";
            break;
        }
        count++;
    }
    return 0;
}
