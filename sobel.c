#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#pragma pack(1)

#define CELL_A i-1][j-1
#define CELL_B i-1][j
#define CELL_C i-1][j+1
#define CELL_D i][j-1
#define CELL_E i][j
#define CELL_F i][j+1
#define CELL_G i+1][j-1
#define CELL_H i+1][j
#define CELL_I i+1][j+1

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
	unsigned char blue;
	unsigned char green;
	unsigned char red;
};
typedef struct pixel Pixel;

int main(int argc, char **argv){

    Header h;
	FILE *arquivoEntrada, *arquivoSaida;
	arquivoEntrada = arquivoSaida = NULL;
	char nomeArquivo[50], saida[100], nomeSemExtensao[50];

    if (argc == 3) {
		// TODO Fazer a inicialização com processos e tudo
    } else if (argc <= 1) {

		strcpy(nomeArquivo, "Borboleta.bmp");
		strcpy(nomeSemExtensao, nomeArquivo);
	}

	nomeSemExtensao[strlen(nomeSemExtensao) - 4] = '\0';
	strcpy(saida, nomeSemExtensao);
	strcat(saida, "_Greyscale");

	arquivoEntrada = fopen(nomeArquivo, "rb");
	if (arquivoEntrada == NULL) {
        printf("Erro ao abrir a imagem de entrada.\n");
        exit(0);
	}

	arquivoSaida = fopen(saida, "wb");
	if (arquivoSaida == NULL) {
		printf("Erro ao salvar a imagem em tons de cinza.\n");
		exit(0);
	}
	
	fread(&h, sizeof(Header), 1, arquivoEntrada);

	int i, j;
	unsigned char grey;

	// Criar as matriz de Pixels e o valor Greyscale de cada Pixel
	// TODO Criar em uma área de memória compartilhada
	Pixel **imagem = (Pixel**) malloc(h.altura * sizeof(Pixel *));
	unsigned char **greyscale = (unsigned char**) malloc(h.altura * sizeof(unsigned char *));
	unsigned char **sobel = (unsigned char**) malloc(h.altura * sizeof(unsigned char *));

	for(i=0; i<h.altura; i++) {
		imagem[i] = (Pixel*) malloc(h.largura * sizeof(Pixel)); 
		greyscale[i] = (unsigned char*) malloc(h.largura * sizeof(unsigned char));
		sobel[i] = (unsigned char*) malloc(h.largura * sizeof(unsigned char));
	}

	// int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	// Ler todos os pixels da imagem
	for(i=0; i<h.altura; i++) {


		for(j=0; j<h.largura; j++) {

			fread(&imagem[i][j], sizeof(Pixel), 1, arquivoEntrada);

			// Converter pra Grayscale
			grey = imagem[i][j].red * 0.2126 + imagem[i][j].green * 0.7152 + imagem[i][j].blue * 0.0722;
			imagem[i][j].red = imagem[i][j].green = imagem[i][j].blue = grey;
			greyscale[i][j] = grey;
		}

		// fseek(arquivoEntrada, alinhamento, SEEK_CUR);
	}

	fclose(arquivoEntrada);

	// Gerar um arquivo Greyscale
	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	for (i=0; i<h.altura; i++) {

		for (j=0; j<h.largura; j++) {
			fwrite(&imagem[i][j], sizeof(Pixel), 1, arquivoSaida);
		}

		// for (j=0; j<alinhamento; j++) {
		// 	fputc(0x00, arquivoSaida);
		// }
	}

	fclose(arquivoSaida);

	long double gx, gy;
	double p;
	// Aplicação do filtro de Sobel
	for (i=1; i<h.altura - 1; i++) {

		for (j=1; j<h.largura - 1; j++) {

			gx = 
				greyscale[CELL_A] * -1 + greyscale[CELL_B] * 0 + greyscale[CELL_C] * 1
			+ 	greyscale[CELL_D] * -2 + greyscale[CELL_E] * 0 + greyscale[CELL_F] * 2
			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * 0 + greyscale[CELL_I] * 1;

			gy = 
				greyscale[CELL_A] * 1  + greyscale[CELL_B] * 2  + greyscale[CELL_C] * 1
			+ 	greyscale[CELL_D] * 0  + greyscale[CELL_E] * 0  + greyscale[CELL_F] * 0
			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * -2 + greyscale[CELL_I] * -1;

			p = sqrt(gx*gx + gy*gy);
			sobel[i][j] = p;
		}
	}

	
	strcpy(saida, nomeSemExtensao);
	strcat(saida, "_Sobel");
	arquivoSaida = fopen(saida, "wb");

	fwrite(&h, sizeof(Header), 1, arquivoSaida);


	// Pixel temp;
	for (i=0; i<h.altura; i++) {

		for (j=0; j<h.largura; j++) {
			fwrite(&sobel[i][j], sizeof(Pixel), 1, arquivoSaida);
		}

		// for (j=0; j<alinhamento; j++) {
		// 	fputc(0x00, arquivoSaida);
		// }
		
	}

	fclose(arquivoSaida);

	return 0;
}