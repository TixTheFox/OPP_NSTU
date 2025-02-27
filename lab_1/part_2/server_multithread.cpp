#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include <cstring>
#include <signal.h> 

#define STACK_SIZE 30 * 1024 * 1024

char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
"<style>body { background-color: #111 }"
"h1 { font-size:4cm; text-align: center; color: black;"
" text-shadow: 0 0 2mm red}</style></head>"
"<body><h1>Goodbye, world!</h1></body></html>\r\n";


void* acceptQuery(void* arg) {
    int* client_fd = (int*) arg;
    char buffer[1024];
    read(*client_fd, buffer, 1024);
    // usleep(50); 
    std::cout << "thread accept " << *client_fd << "\n";
    send(*client_fd, response, sizeof(response), 0);
    close(*client_fd);
    delete client_fd;
}


int main() {
    // бинарная единичка - чтобы устанавливать true некоторым флагам
    int one = 1;
    // хранит дескриптор подключенного клиента
    int client_fd;
    // структуры для хранения сетевой информации о сервере и клиенте
    struct sockaddr_in svr_addr, cli_addr;
    // размер структуры для хранения сетевой информации о клиенте
    socklen_t sin_len = sizeof(cli_addr);

    /* 
    создаем сокет с параметрами
        AF_INET - для использования IPv4
        SOCK_STREAM - для использования TCP
    В случае ошибки сообщаем об этом с завершением программы
    */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cout << "Невозможно создать сокет";
        exit(-1);
    }

    // устанавливаем дополнительный параметр SO_REUSEADDR - при перезапуске сервера
    // позволяет избежать ошибки "Address already in use"
    // setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

    // устанавливаем параметры сервера
    // порт
    int port = 8081;
    svr_addr.sin_port = htons(port);
    // использование IPv4 адресов
    svr_addr.sin_family = AF_INET;
    // разрешаем принимать подключения со всех интерфейсов
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    
    // привязываем заданные параметры к созданному сокету
    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
        close(sock);
        std::cout << "Невозможно связать сокет с его параметрами";
    }

    pthread_t thread;
    pthread_attr_t thread_attr;
    int err = 0;

    err = pthread_attr_init(&thread_attr);
    if (err != 0) {
        std::cout << "Невозможно инициализировать атрибуты потока: " << strerror(err) << "\n";
        exit(-1);
    }

    err = pthread_attr_setstacksize(&thread_attr, STACK_SIZE);
    if (err != 0) {
        std::cout << "Невозможно задать размер стека потока: " << strerror(err) << "\n";
        exit(-1);
    }
    // запускаем прослушивание сокета sock с заданным размером очереди подключений 
    listen(sock, 1000);
    while (1) {
        client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
        if (client_fd == -1) {
            perror("Can't accept");
            continue;
        }
        
        int* thread_client_fd = new int;
        *thread_client_fd = client_fd;

        err = pthread_create(&thread, &thread_attr, acceptQuery, (void *) thread_client_fd);
        pthread_detach(thread);
    }
}