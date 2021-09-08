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

	// Criar a matriz de Pixels
	// TODO Criar em uma área de memória compartilhada
	Pixel **imagem = (Pixel**) malloc(h.altura * sizeof(Pixel *));
	for(i=0; i<h.altura; i++) {
		imagem[i] = (Pixel*) malloc(h.largura * sizeof(Pixel)); 
	}

	// Ler todos os pixels da imagem
	for(i=0; i<h.altura; i++) {
		for(j=0; j<h.largura; j++) {
			fread(&imagem[i][j], sizeof(Pixel), 1, arquivoEntrada);

			// Converter pra Grayscale
			imagem[i][j].red = (unsigned char) imagem[i][j].red * 0.2126;
			imagem[i][j].green = (unsigned char) imagem[i][j].green * 0.7152;
			imagem[i][j].blue = (unsigned char) imagem[i][j].blue * 0.0722;

		}
	}

	fclose(arquivoEntrada);
}