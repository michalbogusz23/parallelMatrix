#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define NUM_OF_THREADS 6
typedef struct Pair
{
    int a;
    int b;
}Pair;
typedef struct MultiplyThread{
    double**A;
    int a;
    int b;
    double**B;
    int c;
    int d;
    double**C;
    Pair indexRange;
    double *partialSum;
    pthread_mutex_t *sumMutex;
} MultiplyThread;
void mnoz(double**A, int a, int b, double**B, int c, int d, double**C)
{
    int i, j, k;
    double s;
    for(i=0; i<a; i++)
    {
        for(j=0; j<d; j++)
        {
            s = 0;
            for(k=0; k<b; k++)
            {
                s+=A[i][k]*B[k][j];
            }
            C[i][j] = s;
        }

    }
}
void* multiplyMatrix(void* args){
    MultiplyThread *data = args;
    double **A = data->A;
    int a = data->a;
    int b = data->b;
    double **B = data->B;
    int c = data->c;
    int d = data->d;
    double **C = data->C;
    Pair indexRange = data->indexRange;
    double *partialSum = data->partialSum;
    pthread_mutex_t *sumMutex = data->sumMutex;

    /*podczad dzielenia macierzy na drobne podmacierze
    otrzymujemy indeksy dla tablicy jednowymiarowej
    jednak korzystamy z tablicy dwuwymiarowej dlatego tak duzo
    obliczen pomiedzy indeksami
    Sa zawile ale dzialaja!!!
    */
    int i, j, jcord, k;
    int distance = indexRange.b - indexRange.a;
    double s;
    int startPointForA = (int)floor((double)indexRange.a / (double)d);
    int endPointForA   = (int)ceil((double)indexRange.b / (double)d);
    for(i=startPointForA; i<endPointForA; i++)
    {
        jcord=indexRange.a % d;
        for(j=0; j<distance; j++)
        {
            s = 0;
            for(k=0; k<b; k++)
            {
                s+=A[i][k]*B[k][jcord];
            }
            C[i][jcord] = s;

            if(partialSum){
            pthread_mutex_lock(sumMutex);
            *partialSum += s;
            pthread_mutex_unlock(sumMutex);
            }
            jcord++;
            if(jcord>=d){
                jcord = jcord%d;
                i++;
            }
            //Koniec zawilych obliczen z indeksami
        }
    }


    return 0;
}
Pair *divideMatrixOnPieces(int sizeOfMatrix) {
    Pair *pieces = malloc(NUM_OF_THREADS * sizeof(*pieces));
    int current = 0;
    int length = (int)ceil((double)sizeOfMatrix / (double)NUM_OF_THREADS);
    for(int i=0; i<NUM_OF_THREADS; i++){
        if (current >= sizeOfMatrix)
        {
            pieces[i].a = 0;
            pieces[i].b = 0;
        }
        else
        {
            pieces[i].a = current;
            int min = current + length;
            if(sizeOfMatrix < min)
                min = sizeOfMatrix;
            pieces[i].b = min;
            current = pieces[i].b;
        }
    }
    return pieces;
}
void print_matrix(double**A, int m, int n)
{
    int i, j;
    printf("[");
    for(i =0; i< m; i++)
    {
        for(j=0; j<n; j++)
        {
            printf("%f ", A[i][j]);
        }
        printf("\n");
    }
    printf("]\n");
}
int main()
{
    FILE *fpa;
    FILE *fpb;
    double **A;
    double **B;
    double **C;
    int ma, mb, na, nb;
    int i, j;
    double x;

    fpa = fopen("A.txt", "r");
    fpb = fopen("B.txt", "r");
    if( fpa == NULL || fpb == NULL )
    {
        perror("błąd otwarcia pliku");
        exit(-10);
    }

    fscanf (fpa, "%d", &ma);
    fscanf (fpa, "%d", &na);


    fscanf (fpb, "%d", &mb);
    fscanf (fpb, "%d", &nb);

    printf("pierwsza macierz ma wymiar %d x %d, a druga %d x %d\n", ma, na, mb, nb);

    if(na != mb)
    {
        printf("Złe wymiary macierzy!\n");
        return EXIT_FAILURE;
    }

    /*Alokacja pamięci*/
    A = malloc(ma*sizeof(double));
    for(i=0; i< ma; i++)
    {
        A[i] = malloc(na*sizeof(double));
    }

    B = malloc(mb*sizeof(double));
    for(i=0; i< mb; i++)
    {
        B[i] = malloc(nb*sizeof(double));
    }

    /*Macierz na wynik*/
    C = malloc(ma*sizeof(double));
    for(i=0; i< ma; i++)
    {
        C[i] = malloc(nb*sizeof(double));
    }

    printf("Rozmiar C: %dx%d\n", ma, nb);
    for(i =0; i< ma; i++)
    {
        for(j = 0; j<na; j++)
        {
            fscanf( fpa, "%lf", &x );
            A[i][j] = x;
        }
    }

    printf("A:\n");
    print_matrix(A, ma, mb);

    for(i =0; i< mb; i++)
    {
        for(j = 0; j<nb; j++)
        {
            fscanf( fpb, "%lf", &x );
            B[i][j] = x;
        }
    }

    printf("B:\n");
    print_matrix(B, mb, nb);

    //poczatek tworzenia i incjalizowania watkow
    Pair *pieces  = divideMatrixOnPieces(ma*nb);
    printf("Podzial na indeksy: \n");
    for(int i = 0; i < NUM_OF_THREADS; i++){
        printf("%d %d\n", pieces[i].a, pieces[i].b);
    }
    double sum = 0;
    pthread_mutex_t sumMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t id[NUM_OF_THREADS];
    MultiplyThread threadArg[NUM_OF_THREADS];

	for (int i=0; i < NUM_OF_THREADS; i++) {
        threadArg[i].A = A;
        threadArg[i].a = ma;
        threadArg[i].b = na;
        threadArg[i].B = B;
        threadArg[i].c = mb;
        threadArg[i].d = nb;
        threadArg[i].C = C;
        threadArg[i].indexRange = pieces[i];
        threadArg[i].partialSum = &sum;
        threadArg[i].sumMutex = &sumMutex;
		if(pthread_create(&id[i], NULL, multiplyMatrix, &threadArg[i]))
		 {
            printf("Can not create a thread\n");
            exit(EXIT_FAILURE);
        }
	}

    for (i=0; i < NUM_OF_THREADS; i++) {
		pthread_join(id[i], NULL);
	}
	free(pieces);

	pthread_mutex_destroy(&sumMutex);
	//koniec watkow
	printf("Wynik obliczania macierzy C przy uzyciu watkow: \n");
	print_matrix(C,ma,nb);
	printf("Suma obliczona za pomoca watkow: %f\n", sum);


    mnoz(A, ma, na, B, mb, nb, C);

    printf("C obliczone klasycznie:\n");
    print_matrix(C, ma, nb);


    for(i=0; i<ma; i++)
    {
        free(A[i]);
    }
    free(A);

    for(i=0; i<mb; i++)
    {
        free(B[i]);
    }
    free(B);

    for(i=0; i<ma; i++)
    {
        free(C[i]);
    }
    free(C);


    fclose(fpa);
    fclose(fpb);


    return 0;
}
