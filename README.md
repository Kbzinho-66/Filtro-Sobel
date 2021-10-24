# Filtro-Sobel
Implementação em C dos filtros de Gauss, Sobel e Mediana usando a biblioteca Pthreads para paralelizar o cálculo dos filtros.

Não esquecer de linkar as bibliotecas math e pthreads ao compilar. A flag O3 aumenta severamente o speedup com relação ao número de threads utilizado.

O programa recebe três argumentos, o nome da imagem original com formato .bmp, o tamanho da máscara usada nos filtros de Gauss e Mediana, e o número máximo de threads a utilizar. Normalmente, só a imagem final é salva, mas alterando o "#define EXTRAS" para 1, todas as imagens intermediárias serão salvas, incluindo após a aplicação do filtro de Mediana.