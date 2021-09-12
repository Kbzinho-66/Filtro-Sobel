#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#pragma pack(1)

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

    if (argc != 3) {
        printf("%s [nome da imagem de entrada] [numero de processos]\n", argv[0]);
		// exit(0);
    }

    FILE *arquivoEntrada = fopen(argv[1], "rb");
    if (arquivoEntrada == NULL) {
        printf("Erro ao abrir a imagem de entrada.\n");
        exit(0);
    }

    char saida[100] = "Greyscale_";
    strcat(saida, argv[1]);
    FILE *arquivoSaida = fopen(saida, "wb");

    if (arquivoSaida == NULL) {
        printf("Erro ao abrir a imagem em tons de cinza.\n");
        exit(0);
    }

    fread(&h, sizeof(Header), 1, arquivoEntrada);

    int altura, largura;
	int i, j;
	unsigned char grey;

	// Criar as matriz de Pixels e o valor Greyscale de cada Pixel
	// TODO Criar em uma área de memória compartilhada
	Pixel **imagem = (Pixel**) malloc(h.altura * sizeof(Pixel *));
	unsigned char **greyscale = (unsigned char**) malloc(h.altura * sizeof(unsigned char *));

	for(i=0; i<h.altura; i++) {
		imagem[i] = (Pixel*) malloc(h.largura * sizeof(Pixel)); 
		greyscale[i] = (unsigned char*) malloc(h.largura * sizeof(unsigned char));
	}

	// Ler todos os pixels da imagem
	for(i=0; i<h.altura; i++) {
		for(j=0; j<h.largura; j++) {
			fread(&imagem[i][j], sizeof(Pixel), 1, arquivoEntrada);

			// Converter pra Grayscale
			grey = imagem[i][j].red * 0.2126 + imagem[i][j].green * 0.7152 + imagem[i][j].blue * 0.0722;
			imagem[i][j].red = imagem[i][j].green = imagem[i][j].blue = grey;
			greyscale[i][j] = grey;
		}
	}

	fclose(arquivoEntrada);

	// Gerar um arquivo Greyscale
	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	for (i=0; i<h.altura; i++) {
		for (j=0; j<h.largura; j++) {
			fwrite(&imagem[i][j], sizeof(Pixel), 1, arquivoSaida);
		}
	}

	fclose(arquivoSaida);

	unsigned char gx, gy;
	// Aplicação do filtro de Sobel
	for (i=0; i<h.altura; i++) {
		for (j=0; j<h.largura; j++) {

			if (i > 0 && i < h.altura - 1 && j > 0 && j < h.largura - 1) { // Pixel pra dentro da borda
				gx = 
					greyscale[i-1][j-1] * -1 + greyscale[i-1][j] * 0 + greyscale[i-1][j+1] * 1
				+ 	greyscale[i][j-1] * -2 	 + greyscale[i][j] * 0 	 + greyscale[i][j+1] * 2
				+ 	greyscale[i+1][j-1] * -1 + greyscale[i+1][j] * 0 + greyscale[i+1][j+1] * 1;

				gy = 
					greyscale[i-1][j-1] * 1  + greyscale[i-1][j] * 2  + greyscale[i-1][j+1] * 1
				+ 	greyscale[i][j-1] * 0 	 + greyscale[i][j] * 0 	  + greyscale[i][j+1] * 0
				+ 	greyscale[i+1][j-1] * -1 + greyscale[i+1][j] * -2 + greyscale[i+1][j+1] * -1;
			} 
		}
	}
}