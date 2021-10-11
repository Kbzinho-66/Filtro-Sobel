#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>

#pragma pack(1)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#define CELL_A linha-1 * largura + coluna-1
#define CELL_B linha-1 * largura + coluna
#define CELL_C linha-1 * largura + coluna+1
#define CELL_D linha   * largura + coluna-1
#define CELL_E linha   * largura + coluna
#define CELL_F linha   * largura + coluna+1
#define CELL_G linha+1 * largura + coluna-1
#define CELL_H linha+1 * largura + coluna
#define CELL_I linha+1 * largura + coluna+1

#define  greyscale(i,j) greyscale[i * largura + j]

#define sobel(linha, coluna) sobel[linha * largura + coluna]

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

void leitura_greyscale(FILE* arquivoEntrada, Byte* greyscale, int altura, int largura);

void calcular_sobel(Byte* greyscale, Byte* sobel, int rank, int numProc, int altura, int largura);


int main(int argc, char **argv){

	Header h;
	int numProc;
	FILE* arquivoEntrada = NULL;
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

	fread(&h, sizeof(Header), 1, arquivoEntrada);	
	int altura = h.altura;
	int largura = h.largura;
	int tamanho = altura * largura * sizeof(Byte);
	int alinhamento = (4 - (largura * sizeof(Pixel)) % 4) % 4;
	int i, j;
	Byte* greyscale = NULL;
	Byte* sobel = NULL;
	Pixel temp;

	greyscale = (Byte*) malloc(altura * largura * sizeof(Byte));
	leitura_greyscale(arquivoEntrada, greyscale, altura, largura);

	fclose(arquivoEntrada);

	int chaveShmSobel = 7, idShmSobel;
	idShmSobel = shmget(chaveShmSobel, tamanho, 0600 | IPC_CREAT);
	sobel = shmat(idShmSobel, NULL, 0);

	# pragma region Escrever o arquivo em preto e branco
	FILE* arquivoSaida = NULL;
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
	for (i = 0; i < altura; i++) {
		for (j = 0; j < largura; j++) {
			temp.red = temp.green = temp.blue = greyscale[i* largura + j];
			fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
		}

		for (j = 0; j < alinhamento; j++) {
			fputc(0x00, arquivoSaida);
		}
	}	

	fclose(arquivoSaida);
	# pragma endregion Escrever o arquivo em preto e branco

	// # pragma region Aplicar o filtro de Sobel
	// int pid, rank, p;
	// pid = rank = 0;

	// strcpy(saida, nomeArquivo);
	// saida[strlen(saida) - 4] = '\0';
	// strcat(saida, "_Sobel");

	// arquivoSaida = fopen(saida, "wb");
	// if (arquivoSaida == NULL) {
	// 	printf("Erro ao salvar a imagem filtrada.\n");
	// 	exit(0);
	// }


	// /* Como o rank é usado para indicar a linha e coluna iniciais na função
	//  calcular_sobel e a linha e coluna iniciais devem ser 1, o rank do processo
	//  pai precisa ser 1. Por consequência, o rank do primeiro filho é 2 e 
	//  o último é igual ao número de processos total. */
	
	// rank = 1;

	// for (p = 2; p <= numProc; ++p) {
	// 	pid = fork();
	// 	if (pid == 0) {
	// 		rank = p;
	// 		break;
	// 	}
	// }

	// calcular_sobel(greyscale, sobel, rank, numProc, altura, largura);

	// if (rank == 1) {
	// 	for (p = 2; p <= numProc; ++p) { wait(NULL); }

	// 	fwrite(&h, sizeof(Header), 1, arquivoSaida);

	// 	Pixel temp;
	// 	for (i = 0; i < altura; i++) {
	// 		for (j = 0; j < largura; j++) {
	// 			temp.red = temp.green = temp.blue = sobel(i,j);
	// 			fwrite(&temp, sizeof(Pixel), 1, arquivoSaida);
	// 		}

	// 		for (j = 0; j < alinhamento; j++) {
	// 			fputc(0x00, arquivoSaida);
	// 		}
			
	// 	}

	// 	fclose(arquivoSaida);

	// } else {
	// 	shmdt(sobel);
	// 	return 0;
	// }
	// # pragma endregion Aplicar o filtro de Sobel

	// # pragma region Encerramento
	// // Desconectar e deletar as áreas de memórias compartilhadas
	// shmdt(sobel);
	// shmctl(idShmSobel, IPC_RMID, 0);
	
	// long size;
	// char* buf;
	// char* ptr;

	// size = pathconf(".", _PC_PATH_MAX);


	// if ((buf = (char *)malloc((size_t)size)) != NULL)
	// 	ptr = getcwd(buf, (size_t)size);

	// printf("Os resultados podem ser encontrados em %s\n", ptr);

	// #pragma endregion

	return 0;
}

/**
 * @brief Lê a imagem inteira e salva somente após a conversão pra tons de cinza. 
 * 
 * @param arquivoEntrada O ponteiro pra imagem que vai ser convertida.
 * @param greyscale Um ponteiro pra matriz de Bytes em que o resultado vai ser armazenado.
 * @param altura A altura em pixels da imagem, definida no Header
 * @param largura A largura em pixels da imagem, definida no Header
 */
void leitura_greyscale(FILE *arquivoEntrada, Byte *greyscale, int altura, int largura) {

	int i, j;
	Pixel temp;

	// Ler todos os pixels da imagem e converter pra greyscale
	for(i = 0; i < altura; i++) {
		for(j = 0; j < largura; j++) {
			fread(&temp, sizeof(Pixel), 1, arquivoEntrada);
			greyscale(i,j) = temp.red * 0.2126 + temp.green * 0.7152 + temp.blue * 0.0722;
		}
	}

}

// void calcular_sobel(Byte* greyscale, Byte* sobel, int rank, int numProc, int altura, int largura) {

// 	int linha, coluna;
// 	double gx, gy;
// 	Byte p;

// 	// Aplicação do filtro de Sobel
// 	for (linha = rank; linha < altura - 1; linha++) {
// 		for (coluna = 1; coluna < largura - 1; coluna++) {
			
// 			gx = 
// 				greyscale[CELL_A] * -1 + greyscale[CELL_B] * 0 + greyscale[CELL_C] * 1
// 			+ 	greyscale[CELL_D] * -2 + greyscale[CELL_E] * 0 + greyscale[CELL_F] * 2
// 			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * 0 + greyscale[CELL_I] * 1;

// 			gy = 
// 				greyscale[CELL_A] * 1  + greyscale[CELL_B] * 2  + greyscale[CELL_C] * 1
// 			+ 	greyscale[CELL_D] * 0  + greyscale[CELL_E] * 0  + greyscale[CELL_F] * 0
// 			+ 	greyscale[CELL_G] * -1 + greyscale[CELL_H] * -2 + greyscale[CELL_I] * -1;

// 			p = (Byte) sqrt(gx*gx + gy*gy);
			
// 			sobel(linha, coluna) = p;
// 		}
// 	}
// }