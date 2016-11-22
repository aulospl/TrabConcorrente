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
  int *vetor, *ListaPossiveisPivos;
  int tam_vetor, num_processos;
  int i;



  if(argc != 3)
    return 1;

  tam_vetor = atoi(argv[1]);
  num_processos = atoi(argv[2]);

  vetor = criaVetorAleatorio(tam_vetor);
  if(vetor == NULL)
    return 1;

  ListaPossiveisPivos = (int*)malloc(((num_processos-1)*num_processos)*sizeof(int));
  if(ListaPossiveisPivos == NULL)
    return 1;

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
  }

  /*Garante que todas as threads tenha executado antes de passar para a próxima etapa do algoritmo*/
  #pragma omp barrier

  /*Ordena a lista de possiveis pivos.*/
  QuickSort(ListaPossiveisPivos, 0, ((num_processos-1)*num_processos)-1);

  int comm_sz, my_rank;
  int *pivos;
  int *vetor_part;

  pivos = (int*)malloc(sizeof(int)*num_processos-1);
  if(pivos == NULL)
    return 1;

  i = (tam_vetor/num_processos) + (tam_vetor%num_processos);

  vetor_part = (int*)malloc(sizeof(int)*i);
  if(vetor_part == NULL)
        return 1;

  comm_sz = num_processos;
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if(my_rank == 0){
       //Define vetor de pivos
        for(i=0;i<num_processos-1;i++){
          pivos[i] = ListaPossiveisPivos[num_processos+(i*num_processos/2)-1];
        }

        MPI_Scatter(vetor, num_processos, MPI_INTEGER, vetor_part, (tam_vetor/num_processos) + (tam_vetor&num_processos),
            MPI_INTEGER, 0, MPI_COMM_WORLD);

        MPI_Bcast(pivos, num_processos-1, MPI_INTEGER, my_rank, MPI_COMM_WORLD);
  }

  int j, k;
  for(j=0; j<(tam_vetor/num_processos); j++){
      if(my_rank == 0){
          if(vetor_part[j] > pivos[my_rank]){
              MPI_Bcast(&vetor_part[j], 1, MPI_INTEGER, my_rank, MPI_COMM_WORLD);
              k = j;
          }
      }else if(my_rank == num_processos-1){
          if(vetor_part[j] < pivos[my_rank]){
              MPI_Bcast(&vetor_part[j], 1, MPI_INTEGER, my_rank, MPI_COMM_WORLD);
              k = j;
          }
      }else{
          if(vetor_part[j] < )
      }
  }

  MPI_Finalize();

  //printVetor(ListaPossiveisPivos, ((num_processos-1)*num_processos));

  printVetor(vetor, tam_vetor);

  return 0;
}
