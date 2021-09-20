#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>

#pragma pack(1)

#define CELL_A i-1 * h.largura + j-1
#define CELL_B i-1 * h.largura + j
#define CELL_C i-1 * h.largura + j+1
#define CELL_D i   * h.largura + j-1
#define CELL_E i   * h.largura + j
#define CELL_F i   * h.largura + j+1
#define CELL_G i+1 * h.largura + j-1
#define CELL_H i+1 * h.largura + j
#define CELL_I i+1 * h.largura + j+1

#define greyscale(linha, coluna) greyscale[linha * h.largura + coluna]

typedef unsigned char Byte;
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

struct pixel{
	Byte blue;
	Byte green;
	Byte red;
};
typedef struct pixel Pixel;

void converter_grayscale(Pixel **imagem, Byte **greyscale, int numProc, char *nomeArq);

void calcular(Pixel **imagem, Byte **greyscale, int rank, int numProc);

Header h;

int main(int argc, char **argv){

	int numProc;
	FILE *arquivoEntrada, *arquivoSaida;
	arquivoEntrada = arquivoSaida = NULL;
	char nomeArquivo[50];
	int i, j;

    if (argc == 3) {
		strcpy(nomeArquivo, argv[1]);
		numProc = atoi(argv[2]);
    } else if (argc <= 1) {
		strcpy(nomeArquivo, "Barco.bmp");
		numProc = 4;
	}
	
	arquivoEntrada = fopen(nomeArquivo, "rb");
	if (arquivoEntrada == NULL) {
        printf("Erro ao abrir a imagem de entrada.\n");
        exit(0);
	}

	fread(&h, sizeof(Header), 1, arquivoEntrada);	

	int chaveShmGreyscale = 13, idShmGreyscale;
	// int chaveShmSobel = 17, idShmSobel;

	// Criar as matriz de Pixels e o valor Greyscale de cada Pixel
	Pixel **imagem = (Pixel**) malloc(h.altura * sizeof(Pixel *));
	
	
	idShmGreyscale = shmget(chaveShmGreyscale, h.altura * h.largura, 0600 | IPC_CREAT);
	Byte **greyscale = shmat(idShmGreyscale, 0, 0);


	for (i = 0; i < h.altura; i++) {
		imagem[i] = (Pixel*) malloc (h.largura * sizeof(Pixel));
		greyscale[i] = (Byte*) malloc (h.largura * sizeof(Byte));
	}
	// idShmSobel = shmget(chaveShmSobel, h.altura * h.largura, 0600 | IPC_CREAT);
	// Byte *sobel = shmat(idShmSobel, 0, 0);

	// int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	// Ler todos os pixels da imagem
	for(i = 0; i < h.altura; i++) {
		for(j = 0; j < h.largura; j++) {
			fread(&imagem[i][j], sizeof(Pixel), 1, arquivoEntrada);
		}
		// fseek(arquivoEntrada, alinhamento, SEEK_CUR);
	}

	fclose(arquivoEntrada);

	converter_grayscale(imagem, greyscale, numProc, nomeArquivo);

	// long double gx, gy;
	// double p;
	// // Aplicação do filtro de Sobel
	// for (i=1; i<h.altura - 1; i++) {

	// 	for (j=1; j<h.largura - 1; j++) {

	// 		gx = 
	// 			greyscale[CELL_A] * -1 + greyscale[CELL_B] * 0 + greyscale[CELL_C] * 1
	// 		+ 	greyscale[CELL_D] * -2 + greyscale[CELL_E] * 0 + greyscale[CELL_F] * 2
	// 		+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * 0 + greyscale[CELL_I] * 1;

	// 		gy = 
	// 			greyscale[CELL_A] * 1  + greyscale[CELL_B] * 2  + greyscale[CELL_C] * 1
	// 		+ 	greyscale[CELL_D] * 0  + greyscale[CELL_E] * 0  + greyscale[CELL_F] * 0
	// 		+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * -2 + greyscale[CELL_I] * -1;

	// 		p = sqrt(gx*gx + gy*gy);
	// 		sobel[CELL_E] = p;
	// 	}
	// }

	
	// strcpy(saida, nomeSemExtensao);
	// strcat(saida, "_Sobel");
	// arquivoSaida = fopen(saida, "wb");

	// fwrite(&h, sizeof(Header), 1, arquivoSaida);


	// // Pixel temp;
	// for (i=0; i<h.altura; i++) {

	// 	for (j=0; j<h.largura; j++) {
	// 		fwrite(&sobel[i][j], sizeof(Pixel), 1, arquivoSaida);
	// 	}

	// 	// for (j=0; j<alinhamento; j++) {
	// 	// 	fputc(0x00, arquivoSaida);
	// 	// }
		
	// }

	// fclose(arquivoSaida);

	return 0;
}

void converter_grayscale(Pixel **imagem, Byte **greyscale, int numProc, char *nomeArq) {

	char saida[50];

	strcpy(saida, nomeArq);
	saida[strlen(saida) - 4] = '\0'; // Tirar o .bmp do final
	strcat(saida, "_Greyscale.bmp");

	FILE *arquivoSaida = fopen(saida, "wb");
	if (arquivoSaida == NULL) {
		printf("Erro ao salvar a imagem em tons de cinza.\n");
		exit(0);
	}

	int i, j, p;
	int pid, rank = 0;

	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	for (p = 0; p < numProc; p++) {
		pid = fork();
		if (pid == 0) {
			rank = p;
			break;
		}

		calcular(imagem, greyscale, rank, numProc);
	}

	for (p = 0; p < numProc; p++) { wait(NULL); }

	Pixel temp;
	for (i = 0; i < h.altura; i++) {
		for (j = 0; j < h.largura; j++) {
			temp.red = temp.green = temp.blue = greyscale[i][j];
			// if (j != 0 && i % j == 360) {
				printf("%u\t", greyscale[i][j]);
			// }
			fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
		}
	}

	fclose(arquivoSaida);
}

void calcular(Pixel **imagem, Byte **greyscale, int rank, int numProc) {

	int linha, coluna;
	Byte grey;

	// Converter pra Grayscale
	for (linha = rank; linha < h.altura; linha += numProc) {
		for (coluna = rank; coluna < h.largura; coluna += numProc) {
			grey = imagem[linha][coluna].red * 0.2126 
				 + imagem[linha][coluna].green * 0.7152 
				 + imagem[linha][coluna].blue * 0.0722;
			
			greyscale[linha][coluna] = grey;
		}
	}
}