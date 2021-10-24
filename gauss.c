//===========================================================//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#pragma pack(1)

#define greyscale(i, j) greyscale[i * largura + j]
#define entrada(i, j) entrada[i * largura + j]
#define saida(i, j) saida[i * largura + j]

typedef unsigned char Byte;

#define SALVAR_ETAPAS 1

//===========================================================//
struct header {
    unsigned short tipo;
    unsigned int tamanho_arquivo;
    unsigned short reservado1;
    unsigned short reservado2;
    unsigned int offset;
    unsigned int tamanho_image_header;
    int largura;
    int altura;
    unsigned short planos;
    unsigned short bits_por_pixel;
    unsigned int compressao;
    unsigned int tamanho_imagem;
    int largura_resolucao;
    int altura_resolucao;
    unsigned int numero_cores;
    unsigned int cores_importantes;
}; 
typedef struct header Header;

struct pixel {
    Byte blue;
    Byte green;
    Byte red;
};
typedef struct pixel Pixel;

struct args {
    int id;
    int numThreads;
    int tamMasc;
    int altura, largura;
    Byte* entrada;
    Byte* saida;
};
typedef struct args Args;

//===========================================================//

void* calcula_gauss(void* argumentos);

void* calcula_sobel(void* argumentos);

void escrever_arquivo(int tipo, Byte* matriz, char* nomeOriginal, Header h);

void mostrar_diretorio(void);

//===========================================================//

int main(int argc, char **argv){

    Header h;
    Byte *greyscale, *gauss, *sobel;
    Pixel temp;

    int numThreads;

    FILE *arquivoEntrada = NULL;
    char nomeArquivo[50];

    int i, j, t;
    int tamMasc;

    if (argc == 4) {

        strcpy(nomeArquivo, argv[1]);
        tamMasc = atol(argv[2]);
        numThreads = atol(argv[3]);
        if ( !(tamMasc == 3 || tamMasc == 5 || tamMasc == 7) ) {
            printf("O tamanho da máscara deve ser 3, 5, ou 7.\n");
            exit(0);
        }

    } else {
        printf("%s <Nome do arquivo original> <Tamanho da máscara> <Número máximo de threads> \n", argv[0]);
        exit(0);
    }

    arquivoEntrada = fopen(nomeArquivo, "rb");
    if (arquivoEntrada == NULL) {
        printf("Erro ao abrir a imagem de entrada.\n");
        exit(0);
    }

    // Leitura do arquivo e conversão pra Greyscale
    fread(&h, sizeof(Header), 1, arquivoEntrada);	
    int altura  = h.altura;
    int largura = h.largura;
    int tamanho = altura * largura * sizeof(Byte);

    greyscale = (Byte*) malloc(tamanho * sizeof(Byte));
    gauss     = (Byte*) malloc(tamanho * sizeof(Byte));
    sobel     = (Byte*) malloc(tamanho * sizeof(Byte));

    for(i = 0; i < h.altura; i++) {
        for(j = 0; j < h.largura; j++) {
            fread(&temp, sizeof(Pixel), 1, arquivoEntrada);
            greyscale(i, j) = temp.red * 0.2126 + temp.green * 0.7152 + temp.blue * 0.0722;
        }
    }
    fclose(arquivoEntrada);

    if (SALVAR_ETAPAS) {
        escrever_arquivo(1, greyscale, nomeArquivo, h);
    }

    // Inicialização dos Threads
    Args *argumentos    = NULL;
    pthread_t *threadID = NULL;

    argumentos = (Args *)      malloc(numThreads * sizeof(Args));
    threadID   = (pthread_t *) malloc(numThreads * sizeof(pthread_t));

    for (t = 0; t < numThreads; t++) {

        argumentos[t].id 		 = t;
        argumentos[t].numThreads = numThreads;
        
        argumentos[t].tamMasc = tamMasc;
        argumentos[t].altura  = h.altura;
        argumentos[t].largura = h.largura;

        argumentos[t].entrada = greyscale;
        argumentos[t].saida   = gauss;
    }

    // Aplicação do Filtro de Gauss
    for (t = 0; t < numThreads; t++) {
        pthread_create(&threadID[t], NULL, calcula_gauss, (void*) &argumentos[t]);
    }

    for (t = 0; t < numThreads; t++) {
        pthread_join(threadID[t], NULL);
    }

    if (SALVAR_ETAPAS) {
        escrever_arquivo(2, gauss, nomeArquivo, h);
    }

    // Aplicação do Filtro de Sobel sobre a matriz de Gauss
    for (t = 0; t < numThreads; t++) {
        argumentos[t].entrada = gauss;
        argumentos[t].saida   = sobel;
        pthread_create(&threadID[t], NULL, calcula_sobel, (void*) &argumentos[t]);
    }

    for (t = 0; t < numThreads; t++) {
        pthread_join(threadID[t], NULL);
    }

    escrever_arquivo(3, sobel, nomeArquivo, h);

    // Aplicação do Filtro de Sobel sobre a matriz em Greyscale, só para comparação
    if (SALVAR_ETAPAS) {
        for (t = 0; t < numThreads; t++) {
            argumentos[t].entrada = greyscale;
            pthread_create(&threadID[t], NULL, calcula_sobel, (void*) &argumentos[t]);
        }

        for (t = 0; t < numThreads; t++) {
            pthread_join(threadID[t], NULL);
        }

        escrever_arquivo(4, sobel, nomeArquivo, h);
    }

    mostrar_diretorio();

    free(greyscale);
    free(gauss);
    free(sobel);

    return 0;
}

//===========================================================//

void * calcula_gauss(void* argumentos) {

    Args *p = (Args*) argumentos;

    int linha, coluna, i, j;
    int distancia  = (p->tamMasc - 1) / 2; // Distância das bordas que o filtro pode pegar
    int posInicial = p->id + distancia;
    int altura     = p->altura;
    int largura    = p->largura;
    int tamMasc    = p->tamMasc;

    int div, *mascara;
    int mascara3[3][3] = {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    };

    int mascara5[5][5] = {
        1, 	4, 	7, 	4, 1,
        4, 16, 26, 16, 4,
        7, 26, 41, 26, 7,
        4, 16, 26, 16, 4,
        1, 	4, 	7, 	4, 1
    };

    int mascara7[7][7] = {
        0, 	0, 	1, 	 2,  1,  0, 0,
        0, 	3, 13, 	22, 13,  3, 0,
        1, 13, 59, 	97, 59, 13, 1,
        2, 22, 97, 159, 97, 22, 2,
        1, 13, 59, 	97, 59, 13, 1,
        0, 	3, 13, 	22, 13,  3, 0,
        0, 	0, 	1, 	 2,  1,  0, 0
    };

    if (tamMasc == 3) {
        div = 16;
        mascara = (int*) mascara3;
    } else if (tamMasc == 5) {
        div = 273;
        mascara = (int*) mascara5;
    } else {
        div = 1003;
        mascara = (int*) mascara7;
    }

    Byte soma, valorOrig, valorMasc;

    for (linha = posInicial; linha < altura - distancia - 1; linha += p->numThreads) {
        for (coluna = distancia; coluna < largura - distancia - 1; coluna++) {

            soma = 0;

            for (i = -distancia; i < tamMasc - distancia; i++) {
                for (j = -distancia; j < tamMasc - distancia; j++) {
                    valorOrig = p->entrada[ (linha + i) * largura + (coluna + j) ];
                    valorMasc = mascara[ (i + distancia) * tamMasc + (j + distancia) ];
                    soma += valorOrig * valorMasc / div;
                }
            }

            p->saida[linha * p->largura + coluna] = (Byte) soma; 

        }
    }

    return NULL;
}

void * calcula_sobel(void* argumentos) {

    Args *p = (Args*) argumentos;

    int linha, coluna;
    int i, j;
    int rank 	   = p->id;
    int numThreads = p->numThreads;
    int altura     = p->altura;
    int largura    = p->largura;

    int mascaraX[3][3] = {
        -1, 0, 1,
        -2, 0, 2,
        -1, 0, 1
    };

    int mascaraY[3][3] = {
         1,	 2,  1,
         0,  0,  0,
        -1, -2, -1
    };

    double gx, gy;
    Byte temp, valorOrig;

    // Aplicação do filtro de Sobel
    for (linha = rank + 1; linha < altura - 1; linha += numThreads) {
        for (coluna = 1; coluna < largura - 1; coluna++) {
            
            gx = gy = 0;
            for (i = -1; i < 2; i++) {
                for (j = -1; j < 2; j++) {
                    valorOrig = p->entrada[ (linha + i) * largura + (coluna + j) ];
                    gx += valorOrig * mascaraX[i+1][j+1];
                    gy += valorOrig * mascaraY[i+1][j+1];                   
                }
            }

            temp = sqrt(gx*gx + gy*gy);
            
            p->saida(linha, coluna) = temp;
        }
    }

    return NULL;
}

void escrever_arquivo(int tipo, Byte* matriz, char* nomeOriginal, Header h) {

    FILE *arquivoSaida = NULL;
    char saida[50];
    Pixel temp;
    int i, j;

    strcpy(saida, nomeOriginal);
    saida[strlen(saida) - 4] = '\0'; // Tirar o .bmp do final
    switch (tipo) {
        case 1:
            strcat(saida, "_Greyscale");
            break;
        case 2:
            strcat(saida, "_Gauss");
            break;
        case 3:
            strcat(saida, "_SobelComGauss");
            break;
        case 4:
            strcat(saida, "_Sobel");
            break;
    }

    arquivoSaida = fopen(saida, "wb");
    if (arquivoSaida == NULL) {
        printf("Erro ao salvar uma das imagens.\n");
        exit(0);
    }

    fwrite(&h, sizeof(Header), 1, arquivoSaida);
    for (i = 0; i < h.altura; i++) {
        for (j = 0; j < h.largura; j++) {
            temp.red = temp.green = temp.blue = matriz[i * h.largura + j];
            fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
        }

    }	

    fclose(arquivoSaida);
}

void mostrar_diretorio(void) {
    
    long size;
    char *buf;
    char *ptr;

    size = pathconf(".", _PC_PATH_MAX);


    if ((buf = (char *)malloc((size_t)size)) != NULL)
        ptr = getcwd(buf, (size_t)size);

    printf("Os resultados podem ser encontrados em %s\n", ptr);
}