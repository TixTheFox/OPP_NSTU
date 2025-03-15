#ifndef MAPREDUCE_H
#define MAPREDUCE_H

#include <pthread.h>
#include <iostream>
#include <cstring>
#include <map>
#include <list>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }

struct Array {
    int* data;
    size_t size;
};

struct MapArgInfo {
    int* array;  // ссылка на массив
    int start;      // Индекс первого обрабатываемого в потоке элемента массива
    int end;        // Индекс элемента, следующего до последнего обрабатываемого элемента

    int (*map_f)(int);
};

struct ReduceArgInfo {
    list<pair<const int, int>> data;  

    int (*reduce_f)(int);
};

#endif