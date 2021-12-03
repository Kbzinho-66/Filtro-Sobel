
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <float.h>
#include <math.h>

void gera_matriz(double** matriz, size_t dim);

void copia_vetor(double* origem, double* dest, size_t tam);

double calcula_erro(double* ult, double* atu, size_t tam);

int main(int argc, char** argv){

    if (argc != 3) {
        printf("%s <Tamanho da matriz> <Limite de Threads>\n", argv[0]);
        exit(0);
    }

    size_t dimMatriz = atoi(argv[1]);
    if (dimMatriz > 20000) {
        printf("Tamanho máximo excedido, usando 20000.\n");
        dimMatriz = 20000;
    }
    size_t nThreads  = atoi(argv[2]);

    size_t kMax = 150;
    double tolErro = 1e-10;

    double** A = (double**) malloc(dimMatriz * sizeof(double*));
    double b[dimMatriz];
    double x[dimMatriz];

    for (size_t i = 0; i < dimMatriz; ++i) {
        A[i] = (double*) malloc(dimMatriz * sizeof(double));
        b[i] = 1;
        x[i] = 0;
    }

    omp_set_num_threads(nThreads);
    gera_matriz(A, dimMatriz);

    // for (size_t i = 0; i < dimMatriz; i++) {
    //     for (size_t j = 0; j < dimMatriz; j++) {
    //         printf("%lf ", A[i][j]);
    //     }
    //     printf("\n");
    // }

    size_t k = 0;
    double erro = DBL_MAX;
    double u[dimMatriz];

    while (k < kMax && erro > tolErro) {

        for (size_t i = 0; i < dimMatriz; i++) {
            u[i] = x[i];
        }

        #pragma omp parallel 
        {
            size_t id = omp_get_thread_num();
            size_t nThreads = omp_get_num_threads();
            double sigma;

            for (size_t i = id; i < dimMatriz; i+= nThreads) {
                sigma = 0;
                for (size_t j = 0; j < dimMatriz; j++) {
                    if (i != j) sigma += A[i][j] * x[i];
                }
                x[i] = (1 / A[i][i]) * (b[i] - sigma);
            }
        }

        ++k;
        
        erro = calcula_erro(u, x, dimMatriz);

    }
    
    printf("Convergiu em %zu iterações.\n", k);
    if (dimMatriz <= 30) {
        for (size_t i = 0; i < dimMatriz; ++i) {
            printf("%.6lf\n", x[i]);
        }
    }
    printf("Erro: %.8e\n", erro);
    return 0;
}

void gera_matriz(double** matriz, size_t dim) {

    #pragma omp parallel for
    for (size_t i = 0; i < dim; i++) {
        for (size_t j = 0; j < dim; j++) {
            if (i == 0 && j == 0 || i == dim - 1 && j == dim - 1) {
                matriz[i][j] = 6;
            } else if (i == j) {
                matriz[i][j] = 4;
            } else if (j == i - 1 || j == i + 1) {
                matriz[i][j] = 1;
            } else {
                matriz[i][j] = 0;
            }
        }
    }
}

double calcula_erro(double* ult, double* atu, size_t tam) {

    double somaUlt, somaAtu;
    somaUlt = somaAtu = 0;

    for (size_t i = 0; i < tam; i++) {
        somaUlt += ult[i];
    }

    for (size_t i = 0; i < tam; i++) {
        somaAtu += atu[i];
    }


    return fabs(somaUlt - somaAtu) / somaUlt;
}