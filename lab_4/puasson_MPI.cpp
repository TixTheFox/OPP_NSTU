#include <initializer_list>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <mpi.h>
#include <iostream>


#define NX 50      // Глобальный размер сетки по x
#define NY 50      // Глобальный размер сетки по y
#define NZ 80      // Глобальный размер сетки по z
#define MAX_ITER 6000  // Максимальное количество итераций
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
    int rank, size;
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Проверка, что число процессов делит NZ
    if (NZ % size != 0) {
        if (rank == 0) {
            printf("Ошибка: количество процессов (%d) должно делить NZ (%d) без остатка\n", size, NZ);
        }
        MPI_Finalize();
        return 1;
    }
    
    // Определение локального размера (по оси z)
    int local_nz = NZ / size + ((rank == size - 1 || rank == 0) ? 1 : 2);
    
    if (rank == 0) {
        printf("Размер сетки: %d x %d x %d\n", NX, NY, NZ);
        printf("Количество процессов: %d\n", size);
    }
    
    // Выделение памяти для локальных сеток (с граничными ячейками)
    double ***u = new double**[NX+1];
    double ***u_new = new double**[NX+1];
    
    for (int i = 0; i < NX+1; i++) {
        u[i] = new double*[NY+1];
        u_new[i] = new double*[NY+1];
        
        for (int j = 0; j < NY+1; j++) {
            u[i][j] = new double[local_nz];
            u_new[i][j] = new double[local_nz];
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

    // Инициализация локальных сеток
    for (int i = 0; i < NX+1; i++) {
        for (int j = 0; j < NY+1; j++) {
            for (int k = 0; k < local_nz; k++) {
                // Глобальный индекс узла по оси z
                int global_k;
                // для первого слоя все тривиально
                if (rank == 0) global_k = k;
                // для последнего слоя чуть сложнее, но тоже понятно - рассчитываем, используя максимальный индекс
                else if (rank == size - 1) global_k = NZ - local_nz + k + 1;
                /* 
                для промежуточных используем размеры блоков без учета дополнительных слоев-границ (local_nz - 2)
                а так же сдвигаем на 1 назад для учета дополнительного (граничного) элемента
                Пример. 5 процессов, размер сетки по z - 100 элементов, размер одного слоя по z - 20 элементов. 
                У слоя с рангом 1 (второй по счету) элементы с 19 по 40 (нумерация везде с 0),
                тогда у k = 0 имеем global_k = 0 + 1 * (22 - 2) - 1 = 19, k = 21 ==> global_k = 40.
                */
                else global_k = k + rank * (local_nz - 2) - 1;
                
                // Координаты точки
                double x = MINX + i * hx;
                double y = MINY + j * hy;
                double z = MINZ + global_k * hz;
                
                // Граничные условия
                if (i == 0 || i == NX || j == 0 || j == NY || 
                    global_k == 0 || global_k == NZ) {
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
    
    
    // Выделение буферов для обмена данными
    double *send_up = new double[(NX+1) * (NY+1)];
    double *send_down = new double[(NX+1) * (NY+1)];
    double *recv_up = new double[(NX+1) * (NY+1)];
    double *recv_down = new double[(NX+1) * (NY+1)];
    
    // Итерационный процесс (метод Якоби)
    double local_error           // локальная ошибка (макс. ошибка в рамках одного процесса)
    double global_error;         // глобальная ошибка (макс. ошибка среди всех процессов)
    double start_time, end_time; // время работы
    int iter = 0;                // счетчик итерация
    
    // заранее определим используемые в циклах переменные
    int global_k;
    double x, y, z, point_error;
    double ***tmp; // используется для замены местами u и u_new (чтобы не переписывать u_new в u)

    start_time = MPI_Wtime();
    // нижний слой
    if (rank == 0) {
        // определяем соседа (один у крайних слоев)
        int send_rank = rank + 1;
        do {
            // расчет прилигающих к границе значений
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    int local_k = local_nz - 2;
                    global_k = local_k;
                    
                    // Координаты точки
                    x = MINX + i * hx;
                    y = MINY + j * hy;
                    z = MINZ + global_k * hz;

                    u_new[i][j][local_k] = (
                        cx * (u[i+1][j][local_k] + u[i-1][j][local_k]) +
                        cy * (u[i][j+1][local_k] + u[i][j-1][local_k]) +
                        cz * (u[i][j][local_k+1] + u[i][j][local_k-1]) -
                        f(x, y, z)
                    ) / cc; 
                }    
            }

            // подготовка данных для отправки
            for (int i = 0; i <= NX; i++) {
                for (int j = 0; j <= NY; j++) {
                    send_up[i * (NX + 1) + j] = u_new[i][j][local_nz - 2];
                }
            }

            // отправка и получение значений с соседом (используем только буферы send_up и recv_up)
            MPI_Request requests[2];
            MPI_Status statuses[2];    
            MPI_Isend(send_up, (NX+1)*(NY+1), MPI_DOUBLE, send_rank, 0, MPI_COMM_WORLD, &requests[0]);
            MPI_Irecv(recv_up, (NX+1)*(NY+1), MPI_DOUBLE, send_rank, 0, MPI_COMM_WORLD, &requests[1]);
            
            
            // Обновление внутренних точек сетки
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    for (int k = 1; k <= local_nz - 3; k++) {
                        // Глобальный индекс k
                        global_k = k;
                        
                        // Координаты точки
                        x = MINX + i * hx;
                        y = MINY + j * hy;
                        z = MINZ + global_k * hz;
                        
                        // Явный метод Якоби
                        u_new[i][j][k] = (
                            cx * (u[i+1][j][k] + u[i-1][j][k]) +
                            cy * (u[i][j+1][k] + u[i][j-1][k]) +
                            cz * (u[i][j][k+1] + u[i][j][k-1]) -
                            f(x, y, z)
                        ) / cc;
                        
                        // Обновление ошибки
                        point_error = fabs(u_new[i][j][k] - u[i][j][k]);
                        if (point_error > local_error) {
                            local_error = point_error;
                        }
                    }
                }
            }

            // меняем местами u и u_new для след. итерации
            tmp = u; u = u_new; u_new = tmp;

            // дожидаемся выполнения отправки и получения
            MPI_Waitall(2, requests, statuses);

            // записываем полученное 
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    u[i][j][local_nz - 1] = recv_up[i * (NX + 1) + j];
                }
            }

            // Нахождение глобальной максимальной ошибки
            MPI_Allreduce(&local_error, &global_error, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            
            iter++;
            local_error = 0;
        } while (global_error > TOL);
        
    // верхний слой
    } else if (rank == size - 1) {
        // определяем соседа (один у крайних слоев)
        int send_rank = rank - 1;
        do {
            // расчет прилигающих к границе значений
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    int local_k = 1;
                    global_k = NZ - local_nz + local_k + 1;
                    
                    // Координаты точки
                    x = MINX + i * hx;
                    y = MINY + j * hy;
                    z = MINZ + global_k * hz;

                    u_new[i][j][local_k] = (
                        cx * (u[i+1][j][local_k] + u[i-1][j][local_k]) +
                        cy * (u[i][j+1][local_k] + u[i][j-1][local_k]) +
                        cz * (u[i][j][local_k+1] + u[i][j][local_k-1]) -
                        f(x, y, z)
                    ) / cc;

                    point_error = fabs(u_new[i][j][local_k] - u[i][j][local_k]);
                    if (point_error > local_error) {
                        local_error = point_error;
                    }
                }    
            }


            // подготовка данных для отправки (используем только буферы send_down и recv_down)
            for (int i = 0; i <= NX; i++) {
                for (int j = 0; j <= NY; j++) {
                    send_down[i * (NX + 1) + j] = u_new[i][j][1];
                }
            }

            // отправка и получение значений с соседом
            MPI_Request requests[2];
            MPI_Status statuses[2];    
            MPI_Isend(send_down, (NX+1)*(NY+1), MPI_DOUBLE, send_rank, 0, MPI_COMM_WORLD, &requests[0]);
            MPI_Irecv(recv_down, (NX+1)*(NY+1), MPI_DOUBLE, send_rank, 0, MPI_COMM_WORLD, &requests[1]);
            
            
            // Обновление внутренних точек сетки
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    for (int k = 2; k <= local_nz - 2; k++) {
                        // Глобальный индекс k
                        global_k = NZ - local_nz + k + 1;
                        
                        // Координаты точки
                        x = MINX + i * hx;
                        y = MINY + j * hy;
                        z = MINZ + global_k * hz;
                        
                        // Явный метод Якоби
                        u_new[i][j][k] = (
                            cx * (u[i+1][j][k] + u[i-1][j][k]) +
                            cy * (u[i][j+1][k] + u[i][j-1][k]) +
                            cz * (u[i][j][k+1] + u[i][j][k-1]) -
                            f(x, y, z)
                        ) / cc;
                        
                        // Обновление ошибки
                        point_error = fabs(u_new[i][j][k] - u[i][j][k]);
                        if (point_error > local_error) {
                            local_error = point_error;
                        }
                    }
                }
            }

            // меняем местами u и u_new для след. итерации
            tmp = u; u = u_new; u_new = tmp;

            // дожидаемся выполнения отправки и получения
            MPI_Waitall(2, requests, statuses);

            // записываем полученное 
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    u[i][j][0] = recv_down[i * (NX + 1) + j];
                }
            }

            // Нахождение глобальной максимальной ошибки
            MPI_Allreduce(&local_error, &global_error, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            
            iter++;
            local_error = 0;
        } while (global_error > TOL);
        
    // все промежуточные слои
    } else {
        // определяем соседей 
        int up_rank = rank + 1;
        int down_rank = rank - 1;
        do {
            // расчет прилигающих к границе значений
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    for (int k : {1, local_nz - 2}) { 
                        global_k = k + rank * (local_nz - 2) - 1;
                        
                        // Координаты точки
                        x = MINX + i * hx;
                        y = MINY + j * hy;
                        z = MINZ + global_k * hz;

                        u_new[i][j][k] = (
                            cx * (u[i+1][j][k] + u[i-1][j][k]) +
                            cy * (u[i][j+1][k] + u[i][j-1][k]) +
                            cz * (u[i][j][k+1] + u[i][j][k-1]) -
                            f(x, y, z)
                        ) / cc;

                        point_error = fabs(u_new[i][j][k] - u[i][j][k]);
                        if (point_error > local_error) {
                            local_error = point_error;
                        }
                    }
                }
            }


            // подготовка данных для отправки
            for (int i = 0; i <= NX; i++) {
                for (int j = 0; j <= NY; j++) {
                    send_up[i * (NX + 1) + j] = u_new[i][j][local_nz - 2];
                    send_down[i * (NX + 1) + j] = u_new[i][j][1];
                }
            }

            // отправка и получение значений
            MPI_Request requests[4];
            MPI_Status statuses[4];    
            MPI_Isend(send_up, (NX+1)*(NY+1), MPI_DOUBLE, up_rank, 0, MPI_COMM_WORLD, &requests[0]);
            MPI_Isend(send_down, (NX+1)*(NY+1), MPI_DOUBLE, down_rank, 0, MPI_COMM_WORLD, &requests[1]);

            MPI_Irecv(recv_up, (NX+1)*(NY+1), MPI_DOUBLE, up_rank, 0, MPI_COMM_WORLD, &requests[2]);
            MPI_Irecv(recv_down, (NX+1)*(NY+1), MPI_DOUBLE, down_rank, 0, MPI_COMM_WORLD, &requests[3]);
            
            // Обновление внутренних точек сетки
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    for (int k = 2; k <= local_nz - 3; k++) {
                        // Глобальный индекс k
                        global_k = k + rank * (local_nz - 2) - 1;
                        
                        // Координаты точки
                        x = MINX + i * hx;
                        y = MINY + j * hy;
                        z = MINZ + global_k * hz;
                        
                        // Явный метод Якоби
                        u_new[i][j][k] = (
                            cx * (u[i+1][j][k] + u[i-1][j][k]) +
                            cy * (u[i][j+1][k] + u[i][j-1][k]) +
                            cz * (u[i][j][k+1] + u[i][j][k-1]) -
                            f(x, y, z)
                        ) / cc;

                        // Обновление ошибки
                        point_error = fabs(u_new[i][j][k] - u[i][j][k]);
                        if (point_error > local_error) {
                            local_error = point_error;
                        }
                    }
                }
            }

            // меняем местами u и u_new для след. итерации
            tmp = u; u = u_new; u_new = tmp;

            // дожидаемся выполнения отправки и получения
            MPI_Waitall(4, requests, statuses);
            // записываем полученное 
            for (int i = 1; i <= NX - 1; i++) {
                for (int j = 1; j <= NY - 1; j++) {
                    u[i][j][local_nz - 1] = recv_up[i * (NX + 1) + j];
                    u[i][j][0] = recv_down[i * (NX + 1) + j];
                }
            }

            // Нахождение глобальной максимальной ошибки
            MPI_Allreduce(&local_error, &global_error, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            
            iter++;
            local_error = 0;
        } while (global_error > TOL);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    
    if (rank == 0) {
        printf("Итераций: %d, финальная ошибка: %e\n", iter, global_error);
        printf("Время выполнения: %f секунд\n", end_time - start_time);
    }

    // -----
    
    // Проверка точности решения (относительно известного аналитического решения)
    double local_max_error = 0.0;
    double global_max_error;
    
    for (int i = 1; i <= NX -1; i++) {
        for (int j = 1; j <= NY -1; j++) {
            for (int k = 1; k < local_nz - 1; k++) {
                if (rank == 0) global_k = k;
                else if (rank == size - 1) global_k = NZ - local_nz + k + 1;
                else global_k = k + rank * (local_nz - 2) - 1;
                
                // Координаты точки
                double x = MINX + i * hx;
                double y = MINY + j * hy;
                double z = MINZ + global_k * hz;
                
                double exact = x*x + y*y + z*z;
                double error = fabs(u[i][j][k] - exact);

                if (error > local_max_error) {
                    local_max_error = error;
                }
            }
        }
    }
    
    // Нахождение глобальной максимальной ошибки
    MPI_Reduce(&local_max_error, &global_max_error, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        printf("Максимальная ошибка относительно аналитического решения: %e\n", global_max_error);
    }

    MPI_Finalize();
    
    return 0;
}