//===========================================================//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#pragma pack(1)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#define CELL_A linha-1][coluna-1
#define CELL_B linha-1][coluna
#define CELL_C linha-1][coluna+1
#define CELL_D linha  ][coluna-1
#define CELL_E linha  ][coluna
#define CELL_F linha  ][coluna+1
#define CELL_G linha+1][coluna-1
#define CELL_H linha+1][coluna
#define CELL_I linha+1][coluna+1

#define resultado(i, j) resultado[i * largura + j]

typedef unsigned char Byte;

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
	int* mascara;
	Byte* gauss;
	Byte** greyscale;
};
typedef struct args Args;

//===========================================================//

void escrever_greyscale(Byte **greyscale, char *nomeArquivo, Header h);

void calcular_sobel(Byte **greyscale, Byte *sobel, int rank, int numThreads, int altura, int largura);

void escrever_sobel(Byte *sobel, char *nomeArquivo, Header h);

void mostrar_diretorio(void);

//===========================================================//

int main(int argc, char **argv){

	const int mascara3[3][3] = {
		1, 2, 1,
		2, 4, 2,
		1, 2, 1
	};

	const int mascara5[5][5] = {
		1, 	4, 	7, 	4, 1,
		4, 16, 26, 16, 4,
		7, 26, 41, 26, 7,
		4, 16, 26, 16, 4,
		1, 	4, 	7, 	4, 1
	};

	const int mascara7[7][7] = {
		0, 	0, 	1, 	 2,  1,  0, 0,
		0, 	3, 13, 	22, 13,  3, 0,
		1, 13, 59, 	97, 59, 13, 1,
		2, 22, 97, 159, 97, 22, 2,
		1, 13, 59, 	97, 59, 13, 1,
		0, 	3, 13, 	22, 13,  3, 0,
		0, 	0, 	1, 	 2,  1,  0, 0
	};

	Header h;
	Byte **greyscale, *resultado;
	Pixel temp;
	Args argumentos;

	int numThreads, pid, rank, p;

	FILE *arquivoEntrada = NULL;
	char nomeArquivo[50];

	int i, j;
	int tamMasc;

	if (argc == 4) {
		strcpy(nomeArquivo, argv[1]);
		tamMasc = atol(argv[2]);
		numThreads = atol(argv[3]);
		if (tamMasc % 2 != 1 && tamMasc < 3 || tamMasc > 7) {
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

	#pragma region Leitura do arquivo e conversão pra Grayscale

	fread(&h, sizeof(Header), 1, arquivoEntrada);	
	int tamanho = h.altura * h.largura * sizeof(Byte);

	greyscale = (Byte**) malloc(h.altura * sizeof(Byte *));
	for (i = 0; i < h.altura; i++) {
		greyscale[i] = (Byte*) malloc (h.largura * sizeof(Byte));
	}

	// Ler todos os pixels da imagem e converter pra greyscale
	for(i = 0; i < h.altura; i++) {
		for(j = 0; j < h.largura; j++) {
			fread(&temp, sizeof(Pixel), 1, arquivoEntrada);
			greyscale[i][j] = temp.red * 0.2126 + temp.green * 0.7152 + temp.blue * 0.0722;
		}
	}

	fclose(arquivoEntrada);

	# pragma endregion

	# pragma region Aplicação do filtro de Gauss

	for (p = 0; p < numThreads; p++) {

		argumentos.id = p;
		argumentos.numThreads = numThreads;

		if (tamMasc == 3) {
			argumentos.tamMasc = 3;
			argumentos.mascara = &mascara3;
		} else if (tamMasc == 5) {
			argumentos.tamMasc = 5;
			argumentos.mascara = &mascara5;
		} else {
			argumentos.tamMasc = 7;
			argumentos.mascara = &mascara7;
		}

		argumentos.greyscale = greyscale;
		argumentos.gauss = resultado;

	}

	# pragma endregion


	# pragma region Aplicar o filtro de Sobel

	calcular_sobel(greyscale, resultado, rank, numThreads, h.altura, h.largura);

	mostrar_diretorio();

	return 0;
}

//===========================================================//

void escrever_greyscale(Byte **greyscale, char *nomeArquivo, Header h) {

	FILE *arquivoSaida = NULL;
	char saida[50];
	Pixel temp;
	int i, j;
	// int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	strcpy(saida, nomeArquivo);
	saida[strlen(saida) - 4] = '\0'; // Tirar o .bmp do final
	strcat(saida, "_Greyscale");

	arquivoSaida = fopen(saida, "wb");
	if (arquivoSaida == NULL) {
		printf("Erro ao salvar a imagem em tons de cinza.\n");
		exit(0);
	}

	fwrite(&h, sizeof(Header), 1, arquivoSaida);
	for (i = 0; i < h.altura; i++) {
		for (j = 0; j < h.largura; j++) {
			temp.red = temp.green = temp.blue = greyscale[i][j];
			fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
		}

		// for (j = 0; j < alinhamento; j++) {
		// 	fputc(0x00, arquivoSaida);
		// }
	}	

	fclose(arquivoSaida);
}

void calcular_sobel(Byte **greyscale, Byte *resultado, int rank, int numThreads, int altura, int largura) {

	int linha, coluna;
	double gx, gy;
	Byte p;

	// Aplicação do filtro de Sobel
	for (linha = rank; linha < altura - 1; linha++) {
		for (coluna = 1; coluna < largura - 1; coluna++) {
			
			gx = 
				greyscale[CELL_A] * -1 + greyscale[CELL_B] * 0 + greyscale[CELL_C] * 1
			+ 	greyscale[CELL_D] * -2 + greyscale[CELL_E] * 0 + greyscale[CELL_F] * 2
			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * 0 + greyscale[CELL_I] * 1;

			gy = 
				greyscale[CELL_A] * 1  + greyscale[CELL_B] * 2  + greyscale[CELL_C] * 1
			+ 	greyscale[CELL_D] * 0  + greyscale[CELL_E] * 0  + greyscale[CELL_F] * 0
			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * -2 + greyscale[CELL_I] * -1;

			p = (Byte) sqrt(gx*gx + gy*gy);
			
			resultado(linha, coluna) = p;
		}
	}
}

void escrever_sobel(Byte *resultado, char *nomeArquivo, Header h) {

	FILE *arquivoSaida = NULL;
	char saida[50];
	Pixel temp;
	int i, j;
	int largura = h.largura;
	// int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	strcpy(saida, nomeArquivo);
	saida[strlen(saida) - 4] = '\0';
	strcat(saida, "_Sobel");

	arquivoSaida = fopen(saida, "wb");
	if (arquivoSaida == NULL) {
		printf("Erro ao salvar a imagem filtrada.\n");
		exit(0);
	}

	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	for (i = 0; i < h.altura; i++) {
		for (j = 0; j < h.largura; j++) {
			temp.red = temp.green = temp.blue = resultado(i,j);
			fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
		}

		// for (j = 0; j < alinhamento; j++) {
		// 	fputc(0x00, arquivoSaida);
		// }
		
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