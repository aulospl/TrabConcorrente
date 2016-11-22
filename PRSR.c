#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <mpi.h>

#define INTERVALO 10000

int *criaVetorAleatorio(int tam_vetor){
  int *vet;
  int i;

  vet = (int*)malloc(tam_vetor*sizeof(int));
  if(vet == NULL)
    return NULL;

  srand(time(NULL));
  for(i = 0; i<tam_vetor; i++)
    vet[i] = rand()%INTERVALO;

  return vet;
}

void printVetor(int *vetor, int tam_vetor){
  int i;
  for(i=0; i<tam_vetor; i++){
    if(i == tam_vetor-1)
      printf("%d.", vetor[i]);
    else
      printf("%d, ", vetor[i]);
  }
}

void QuickSort(int *vetor, int inicio, int fim){

  int i, j, i_pivo, pivo, aux;

  if((fim - inicio) + 1 <= 1)
    return;

  i_pivo = (inicio + fim)/2;

  pivo = vetor[i_pivo];
  vetor[i_pivo] = vetor[fim];
  vetor[fim] = pivo;

  i = inicio - 1;
  j = fim;

  do{

    do{i++;}while(vetor[i] < pivo);
    do{j--;}while(j >= inicio && vetor[j] >= pivo);

    if(i < j){
      aux = vetor[i];
      vetor[i] = vetor[j];
      vetor[j] = aux;
    }

  }while(i < j);

  vetor[fim] = vetor[i];
  vetor[i] = pivo;

  QuickSort(vetor, inicio, i-1);
  QuickSort(vetor, i+1, fim);

  return;
}

int main(int argc, char *argv[]){
  /*variaveis dos processos mpi.*/
  int comm_sz, my_rank;
  MPI_Request request;
  MPI_Status status;

  int *vetor, *ListaPossiveisPivos, *pivos;
  int tam_vetor, num_processos;
  int i;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  /*Faz a primeira parte usando o openmp.
    Cria as threads e executa todas no
      processo id = 0 do mpi.*/
  if(my_rank == 0){

    /*Caso nao hava os elementos minimos para comecar a ordenacao
      o programa aborta todos os processos.*/
    if(argc != 3)
      MPI_Abort(MPI_COMM_WORLD, 1);

    tam_vetor = atoi(argv[1]);
    num_processos = atoi(argv[2]);

    vetor = criaVetorAleatorio(tam_vetor);
    if(vetor == NULL)
      MPI_Abort(MPI_COMM_WORLD, 1);

    ListaPossiveisPivos = (int*)malloc(((num_processos-1)*num_processos)*sizeof(int));
    if(ListaPossiveisPivos == NULL)
      MPI_Abort(MPI_COMM_WORLD, 1);

    pivos = (int*)malloc((num_processos-1)*sizeof(int));
    if(pivos == NULL)
        MPI_Abort(MPI_COMM_WORLD, 1);

    #pragma omp parallel num_threads(num_processos)
    {
      int inicio, fim;

      inicio = omp_get_thread_num()*(tam_vetor/num_processos);
      fim = (omp_get_thread_num() + 1)*(tam_vetor/num_processos);

      /*Ordena as p partições do vetor usando QuickSort.*/
      if(omp_get_thread_num() == (omp_get_num_threads() - 1))
        QuickSort(vetor, inicio, tam_vetor-1);
      else
        QuickSort(vetor, inicio, fim-1);

      /*escolhe os p*(p-1) possiveis pivos.*/
      #pragma omp parallel for
      for(i=0; i<((num_processos-1)*num_processos); i++)
        ListaPossiveisPivos[i] = vetor[(i*tam_vetor)/(num_processos*num_processos)];

      /*Garante que todas as threads tenha executado antes de passar para a próxima etapa do algoritmo*/
      #pragma omp barrier

      /*Escolhe os pivos que serão usados na comunicação*/
      #pragma omp parallel for
      for(i=0; i<(num_processos-1); i++)
        pivos[i] = ListaPossiveisPivos[(i+1)*num_processos + (num_processos/2) -1];
    }

    /*Garante que todas as threads tenha executado antes de passar para a próxima etapa do algoritmo*/
    #pragma omp barrier

  }

  MPI_Barrier(MPI_COMM_WORLD);

  if(my_rank == 0){

    #pragma omp parallel num_threads(num_processos)
    {
      int inicio, fim;

      inicio = omp_get_thread_num()*(tam_vetor/num_processos);
      fim = (omp_get_thread_num() + 1)*(tam_vetor/num_processos);

      /*Ordena as p partições do vetor usando QuickSort.*/
      if(omp_get_thread_num() == (omp_get_num_threads() - 1))
        QuickSort(vetor, inicio, tam_vetor-1);
      else
        QuickSort(vetor, inicio, fim-1);

      printVetor(vetor, tam_vetor);
    }
  }

  MPI_Finalize();

}
