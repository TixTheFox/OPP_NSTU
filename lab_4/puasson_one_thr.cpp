#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <chrono>

using namespace std;

#define NX 50      // Глобальный размер сетки по x
#define NY 50      // Глобальный размер сетки по y
#define NZ 80      // Глобальный размер сетки по z
#define MAX_ITER 5000  // Максимальное количество итераций
#define TOL 1e-6    // Допустимая погрешность (критерий сходимости)

#define MINX -1.0     // Минимальное значение x
#define MINY -1.0     // Минимальное значение y
#define MINZ -1.0     // Минимальное значение z

#define LX 2.0     // Длина области по x
#define LY 2.0     // Длина области по y
#define LZ 2.0     // Длина области по z

// Функция правой части уравнения Пуассона 
double f(double x, double y, double z) {
    return 6.0;
}

// Граничные условия
double boundary_condition(double x, double y, double z) {
    return x*x + y*y + z*z;
}

int main(int argc, char *argv[]) {
    // Выделение памяти для локальных сетки
    double ***u = new double**[NX+1];
    double ***u_new = new double**[NX+1];
    
    for (int i = 0; i < NX+1; i++) {
        u[i] = new double*[NY+1];
        u_new[i] = new double*[NY+1];
        
        for (int j = 0; j < NY+1; j++) {
            u[i][j] = new double[NZ+1];
            u_new[i][j] = new double[NZ+1];
        }
    }
    
    // Размер шага по осям
    double hx = LX / (NX - 1);
    double hy = LY / (NY - 1);
    double hz = LZ / (NZ - 1);
    
    // Коэффициенты для обновления сетки
    double cx = 1.0/(hx*hx);
    double cy = 1.0/(hy*hy);
    double cz = 1.0/(hz*hz);
    double cc = 2.0*(cx + cy + cz);

    // Инициализация сетки
    for (int i = 0; i < NX+1; i++) {
        for (int j = 0; j < NY+1; j++) {
            for (int k = 0; k < NZ+1; k++) {
                // Координаты точки
                double x = MINX + i * hx;
                double y = MINY + j * hy;
                double z = MINZ + k * hz;
                
                // Граничные условия
                if (i == 0 || i == NX || j == 0 || j == NY || 
                    k == 0 || k == NZ) {
                    u[i][j][k] = boundary_condition(x, y, z);
                    u_new[i][j][k] = u[i][j][k];
                } else {
                    // Внутренние точки - начальное приближение
                    u[i][j][k] = 0.0;
                    u_new[i][j][k] = 0.0;
                }
            }
        }
    }
    
    
    // Итерационный процесс (метод Якоби)
    double error;
    double start_time, end_time;
    int iter = 0;
    
    int global_k;
    double x, y, z;
    double point_error;
    double*** tmp;

    auto t1 = chrono::high_resolution_clock::now();
    do {
        error = 0;

        // Обновление внутренних точек сетки
        for (int i = 1; i <= NX - 1; i++) {
            for (int j = 1; j <= NY - 1; j++) {
                for (int k = 1; k <= NZ - 1; k++) {
                    // Координаты точки
                    x = MINX + i * hx;
                    y = MINY + j * hy;
                    z = MINZ + k * hz;
                    
                    // Явный метод Якоби
                    u_new[i][j][k] = (
                        cx * (u[i+1][j][k] + u[i-1][j][k]) +
                        cy * (u[i][j+1][k] + u[i][j-1][k]) +
                        cz * (u[i][j][k+1] + u[i][j][k-1]) -
                        f(x, y, z)
                    ) / cc;
                    
                    // Обновление ошибки
                    point_error = fabs(u_new[i][j][k] - u[i][j][k]);
                    if (point_error > error) {
                        error = point_error;
                    }
                }
            }
        }

        // меняем сетки местами для следующей итерации
        tmp = u;
        u = u_new;
        u_new = tmp;

        iter++;
    } while (error > TOL);
    
    auto t2 = chrono::high_resolution_clock::now();
    double time_result = chrono::duration<double>(t2 - t1).count();

    printf("Время выполнения: %e\n", time_result);
    printf("Достигнутая точность %e\n", error);

    // Проверка точности решения (для известного аналитического решения)
    error = 0;
    for (int i = 1; i <= NX - 1; i++) {
        for (int j = 1; j <= NY - 1; j++) {
            for (int k = 1; k < NZ - 1; k++) {
                // Координаты точки
                double x = MINX + i * hx;
                double y = MINY + j * hy;
                double z = MINZ + k * hz;
                
                // Аналитическое решение для f(x,y,z) = 6 -> u(x,y,z) = x²+y²+z²
                double exact = x*x + y*y + z*z;
                point_error = fabs(u[i][j][k] - exact);

                if (point_error > error) {
                    error = point_error;
                }
            }
        }
    }
     
    printf("Максимальная ошибка относительно аналитического решения: %e\n", error);
    
    // Освобождение памяти
    for (int i = 0; i < NX+1; i++) {
        for (int j = 0; j < NY+1; j++) {
            delete u[i][j];
            delete u_new[i][j];
        }
    
        delete u[i];
        delete u_new[i];
    }
    delete u;
    delete u_new;
    
    return 0;
}