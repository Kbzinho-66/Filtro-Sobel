#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>

#pragma pack(1)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#define CELL_A linha-1 * h.largura + coluna-1
#define CELL_B linha-1 * h.largura + coluna
#define CELL_C linha-1 * h.largura + coluna+1
#define CELL_D linha   * h.largura + coluna-1
#define CELL_E linha   * h.largura + coluna
#define CELL_F linha   * h.largura + coluna+1
#define CELL_G linha+1 * h.largura + coluna-1
#define CELL_H linha+1 * h.largura + coluna
#define CELL_I linha+1 * h.largura + coluna+1

#define greyscale(linha, coluna) greyscale[linha * h.largura + coluna]
#define sobel(linha, coluna) sobel[linha * h.largura + coluna]

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

void converter_grayscale(Pixel **imagem, Byte *greyscale, int numProc, char *nomeArq);

void calcular_grey(Pixel **imagem, Byte *greyscale, int rank, int numProc);

void aplicar_sobel(Byte *greyscale, Byte *sobel, int numProc, char *nomeArq);

void calcular_sobel(Byte *greyscale, Byte *sobel, int rank, int numProc);

Header h;

int main(int argc, char **argv){

	#pragma region   Abertura dos arquivos e declaração das variáveis
	int numProc;
	FILE *arquivoEntrada, *arquivoSaida;
	arquivoEntrada = arquivoSaida = NULL;
	char nomeArquivo[50];
	int i, j;

    if (argc == 3) {
		strcpy(nomeArquivo, argv[1]);
		numProc = atoi(argv[2]);
    } else if (argc <= 1) {
		strcpy(nomeArquivo, "Borboleta.bmp");
		numProc = 4;
	}
	
	arquivoEntrada = fopen(nomeArquivo, "rb");
	if (arquivoEntrada == NULL) {
        printf("Erro ao abrir a imagem de entrada.\n");
        exit(0);
	}

	fread(&h, sizeof(Header), 1, arquivoEntrada);	

	int chaveShmGreyscale = 5, idShmGreyscale;
	int chaveShmSobel = 7, idShmSobel;
	int tamanho = h.altura * h.largura * sizeof(Byte);

	// Criar as matrizes para a imagem original, em preto e branco, e depois de aplicado o filtro
	Pixel **imagem = (Pixel**) malloc(h.altura * sizeof(Pixel *));
	#pragma endregion

	idShmGreyscale = shmget(chaveShmGreyscale, tamanho, 0600 | IPC_CREAT);
	Byte *greyscale = shmat(idShmGreyscale, NULL, 0);

	idShmSobel = shmget(chaveShmSobel, tamanho, 0600 | IPC_CREAT);
	Byte *sobel = shmat(idShmSobel, NULL, 0);

	for (i = 0; i < h.altura; i++) {
		imagem[i] = (Pixel*) malloc (h.largura * sizeof(Pixel));
	}

	int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	// Ler todos os pixels da imagem
	for(i = 0; i < h.altura; i++) {
		for(j = 0; j < h.largura; j++) {
			fread(&imagem[i][j], sizeof(Pixel), 1, arquivoEntrada);
		}
		fseek(arquivoEntrada, alinhamento, SEEK_CUR);
	}

	fclose(arquivoEntrada);

	converter_grayscale(imagem, greyscale, numProc, nomeArquivo);

	aplicar_sobel(greyscale, sobel, numProc, nomeArquivo);

	int iguais, diferentes;
	iguais = diferentes = 0;
	for (i=1; i<h.altura-1; i++) {
		for (j=1; j<h.largura-1; j++) {
			if (greyscale(i,j) == sobel(i,j)) {
				iguais++;
			} else {
				diferentes++;
			}
		}
	}

	printf("Iguais = %d\nDiferentes = %d\n", iguais, diferentes);

	// Desconectar e deletar as áreas de memórias compartilhadas
	shmctl(idShmGreyscale, IPC_RMID, 0);
	shmctl(idShmSobel, IPC_RMID, 0);

	return 0;
}

void converter_grayscale(Pixel **imagem, Byte *greyscale, int numProc, char *nomeArq) {

	char saida[50];
	int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	strcpy(saida, nomeArq);
	saida[strlen(saida) - 4] = '\0'; // Tirar o .bmp do final
	strcat(saida, "_Greyscale.bmp");

	FILE *arquivoSaida = fopen(saida, "wb");
	if (arquivoSaida == NULL) {
		printf("Erro ao salvar a imagem em tons de cinza.\n");
		exit(0);
	}

	int i, j, p;
	int pid = 0, rank = 0;

	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	for (p = 1; p < numProc; p++) {
		pid = fork();
		if (pid == 0) {
			rank = p;
			break;
		}		
	}

	calcular_grey(imagem, greyscale, rank, numProc);
	
	if (rank != 0) {
		shmdt(greyscale);
	} else {
		for (p = 1; p < numProc; p++) { wait(NULL); }

		Pixel temp;
		for (i = 0; i < h.altura; i++) {
			for (j = 0; j < h.largura; j++) {
				temp.red = temp.green = temp.blue = greyscale(i,j);
				fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
			}

			for (j = 0; j < alinhamento; j++) {
				fputc(0x00, arquivoSaida);
			}
		}	

		fclose(arquivoSaida);
	}

}

void calcular_grey(Pixel **imagem, Byte *greyscale, int rank, int numProc) {

	int linha, coluna;
	Byte grey;

	// Converter pra Grayscale
	for (linha = rank; linha < h.altura; linha += numProc) {
		for (coluna = 1; coluna < h.largura; coluna++) {
			grey = imagem[linha][coluna].red * 0.2126 
				 + imagem[linha][coluna].green * 0.7152 
				 + imagem[linha][coluna].blue * 0.0722;
			
			greyscale(linha,coluna) = grey;
		}
	}
}

void aplicar_sobel(Byte *greyscale, Byte *sobel, int numProc, char *nomeArq) {

	char saida[50];
	strcpy(saida, nomeArq);
	saida[strlen(saida) - 4] = '\0';
	strcat(saida, "_Sobel.bmp");

	FILE *arquivoSaida = fopen(saida, "wb");
	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	int i, j, p;
	int pid = 0, rank = 0;
	int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;

	for (p = 1; p < numProc - 1; p++) {
		fork();
		if (pid == 0) {
			rank = p;
			break;
		}
	}

	calcular_sobel(greyscale, sobel, rank, numProc);

	if (rank == 0) {
		for (p = 1; p < numProc; p++) { wait(NULL); }

		Pixel temp;
		for (i = 0; i < h.altura; i++) {
			for (j = 0; j < h.largura; j++) {
				temp.red = temp.green = temp.blue = sobel(i,j);
				printf("%u\t", sobel(i,j));
				fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
			}

			for (j = 0; j < alinhamento; j++) {
				fputc(0x00, arquivoSaida);
			}
			
		}

		fclose(arquivoSaida);

	}
	
}

void calcular_sobel(Byte *greyscale, Byte *sobel, int rank, int numProc) {

	int linha, coluna;
	double gx, gy;
	Byte p;

	// Aplicação do filtro de Sobel
	for (linha = rank; linha < h.altura - 1; linha++) {
		for (coluna = 1; coluna < h.largura - 1; coluna++) {
			
			gx = 
				greyscale[CELL_A] * -1 + greyscale[CELL_B] * 0 + greyscale[CELL_C] * 1
			+ 	greyscale[CELL_D] * -2 + greyscale[CELL_E] * 0 + greyscale[CELL_F] * 2
			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * 0 + greyscale[CELL_I] * 1;

			gy = 
				greyscale[CELL_A] * 1  + greyscale[CELL_B] * 2  + greyscale[CELL_C] * 1
			+ 	greyscale[CELL_D] * 0  + greyscale[CELL_E] * 0  + greyscale[CELL_F] * 0
			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * -2 + greyscale[CELL_I] * -1;

			p = sqrt(gx*gx + gy*gy);
			
			sobel(linha, coluna) = p;
		}
	}
}