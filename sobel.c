#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>

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

void calcular_sobel(Byte **greyscale, Byte *sobel, int rank, int numProc);

Header h;

int main(int argc, char **argv){

	#pragma region Abertura dos arquivos

	int numProc;
	FILE *arquivoEntrada = NULL;
	char nomeArquivo[50];

	if (argc == 3) {
		strcpy(nomeArquivo, argv[1]);
		numProc = atol(argv[2]);

    } else {
		printf("%s <Nome do arquivo original> <Número máximo de processos>\n", argv[0]);
		exit(0);
	}
	
	arquivoEntrada = fopen(nomeArquivo, "rb");
	if (arquivoEntrada == NULL) {
        printf("Erro ao abrir a imagem de entrada.\n");
        exit(0);
	}

	#pragma endregion 

	#pragma region Leitura do arquivo e conversão pra Grayscale

	fread(&h, sizeof(Header), 1, arquivoEntrada);	

	int chaveShmSobel = 7, idShmSobel;
	int tamanho = h.altura * h.largura * sizeof(Byte);
	int alinhamento = (4 - (h.largura * sizeof(Pixel)) % 4) % 4;
	int i, j;
	Byte **greyscale, *sobel;
	Pixel temp;

	greyscale = (Byte**) malloc(h.altura * sizeof(Byte *));

	idShmSobel = shmget(chaveShmSobel, tamanho, 0600 | IPC_CREAT);
	sobel = shmat(idShmSobel, NULL, 0);

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

	# pragma region Escrever o arquivo em preto e branco
	FILE *arquivoSaida = NULL;
	char saida[50];

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

		for (j = 0; j < alinhamento; j++) {
			fputc(0x00, arquivoSaida);
		}
	}	

	fclose(arquivoSaida);
	# pragma endregion Escrever o arquivo em preto e branco

	# pragma region Aplicar o filtro de Sobel
	int pid, rank, p;
	pid = rank = 0;

	strcpy(saida, nomeArquivo);
	saida[strlen(saida) - 4] = '\0';
	strcat(saida, "_Sobel");

	arquivoSaida = fopen(saida, "wb");
	if (arquivoSaida == NULL) {
		printf("Erro ao salvar a imagem filtrada.\n");
		exit(0);
	}

	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	/* Como o rank é usado para indicar a linha e coluna iniciais na função
	 calcular_sobel e a linha e coluna iniciais devem ser 1, o rank do processo
	 pai precisa ser 1. Por consequência, o rank do primeiro filho é 2 e 
	 o último é igual ao número de processos total. */
	
	rank = 1;

	for (p = 2; p <= numProc; ++p) {
		pid = fork();
		if (pid == 0) {
			rank = p;
			break;
		}
	}

	calcular_sobel(greyscale, sobel, rank, numProc);

	if (rank == 1) {
		for (p = 2; p <= numProc; ++p) { wait(NULL); }

		Pixel temp;
		for (i = 0; i < h.altura; i++) {
			for (j = 0; j < h.largura; j++) {
				temp.red = temp.green = temp.blue = sobel(i,j);
				fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
			}

			for (j = 0; j < alinhamento; j++) {
				fputc(0x00, arquivoSaida);
			}
			
		}

		fclose(arquivoSaida);

	} else {
		shmdt(sobel);
		return 0;
	}
	# pragma endregion Aplicar o filtro de Sobel

	# pragma region Encerramento
	// Desconectar e deletar as áreas de memórias compartilhadas
	shmdt(sobel);
	shmctl(idShmSobel, IPC_RMID, 0);
	
	long size;
	char *buf;
	char *ptr;

	size = pathconf(".", _PC_PATH_MAX);


	if ((buf = (char *)malloc((size_t)size)) != NULL)
		ptr = getcwd(buf, (size_t)size);

	printf("Os resultados podem ser encontrados em %s\n", ptr);

	#pragma endregion

	return 0;
}

void calcular_sobel(Byte **greyscale, Byte *sobel, int rank, int numProc) {

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

			p = (Byte) sqrt(gx*gx + gy*gy);
			
			sobel(linha, coluna) = p;
		}
	}
}