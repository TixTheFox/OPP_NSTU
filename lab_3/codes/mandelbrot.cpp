#include <stdio.h>
#include <math.h>
#include <iostream>
#include <omp.h>

using namespace std;

int main(int argc, char* argv[]) {
    // проверки аргументjd
    if (argc != 2) {
        cout << "Ожидался 1 аргумент, получено: " << argc - 1 << "\n";
        exit(-1);
    }
    
    int thread_count = 0;   // число потоков, решающих задачу

    if ( ((thread_count = atoi(argv[1])) == 0) || thread_count <= 0) {
        cout << "Число потоков должно быть целым положительным числом\n";
        exit(-1);
    }

    /* Размер экрана в пикселях*/
    int iX,iY;
    const int iXmax = 1600; 
    const int iYmax = 1600;
    /* Область определения */
    double Cx,Cy;
    const double CxMin=-2.5;
    const double CxMax=1.5;
    const double CyMin=-2.0;
    const double CyMax=2.0;
    /* "Размеры" пикселя в заданной области определения*/
    double PixelWidth=(CxMax-CxMin)/iXmax;
    double PixelHeight=(CyMax-CyMin)/iYmax;
    /*некотрые настройки цвета и файла*/
    const int MaxColorComponentValue=255; 
    FILE * fp;
    char *filename="mandelbrot.ppm";
    char *comment="# ";/* comment should start with # */
    
    double Zx, Zy;   // Z=Zx+Zy*i  ;  Z0 = 0 
    double Zx2, Zy2; // Zx2=Zx*Zx;  Zy2=Zy*Zy  
    /*  */
    int Iteration;
    const int IterationMax=200;

    /* граница выхода из цикла (см. описание множества Мандельброта; показано, что если |zn| > 2,
       то zn не приндлежит множеству)*/
    const double EscapeRadius = 2;
    double ER2=EscapeRadius*EscapeRadius;

    fp= fopen(filename,"wb"); /* b -  binary mode */
    /*Заголовок ppm файла*/
    fprintf(fp,"P6\n %s\n %d\n %d\n %d\n",comment,iXmax,iYmax,MaxColorComponentValue);
    /* вычисление точек */
    unsigned char buffer[3 * iXmax * iYmax];

    // устанавливаем число создаваемых потоков
    omp_set_num_threads(thread_count);

    #pragma omp parallel private(iX, iY, Cx, Cy, Zx, Zy, Zx2, Zy2, Iteration) 
    {
    unsigned char color[3];
    #pragma omp for collapse(2) schedule(dynamic)
    for(iY=0; iY<iYmax; iY++) {
        for(iX=0; iX<iXmax; iX++) {
            Cy=CyMin + iY*PixelHeight; 
            if (fabs(Cy)< PixelHeight/2) Cy=0.0; /* Main antenna */    
            Cx=CxMin + iX*PixelWidth;
            /* начальное значение Z=0 */
            Zx=0.0;
            Zy=0.0;
            Zx2=Zx*Zx;
            Zy2=Zy*Zy;
            for (Iteration=0; Iteration<IterationMax && ((Zx2+Zy2)<ER2); Iteration++) {
                Zy=2*Zx*Zy + Cy;
                Zx=Zx2-Zy2 + Cx;
                Zx2=Zx*Zx;
                Zy2=Zy*Zy;
            };
            // устанавливаем цвет точки в зависимости от результата
            // если принадлежит, то черный (0, 0, 0) 
            if (Iteration==IterationMax) {
                color[0]=0;
                color[1]=0;
                color[2]=0;                           
            }
            else { 
                // иначе - белый (255, 255, 255)
                color[0]=255;
                color[1]=255;
                color[2]=255;
            };

            buffer[3*(iX + iY * iXmax)] = color[0];
            buffer[3*(iX + iY * iXmax) + 1] = color[1];
            buffer[3*(iX + iY * iXmax) + 2] = color[2];
        }
    }
    } // /omp parallel
    fwrite(buffer, 1, 3 * iXmax * iYmax, fp);
    fclose(fp);
    return 0;
}