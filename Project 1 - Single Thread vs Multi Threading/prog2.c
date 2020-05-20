#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define size 10000

int MatrixA[size][size], MatrixB[size][size], MatrixC[size][size];
pthread_t threadID[25];

//function to write the resultant matrix C
void WriteMatrixC()
{
	FILE *fptr;
	int i, j = 0;
	if ((fptr = fopen("MatrixCMT.txt", "w")) == NULL)
	{
		puts("Cannot open file");
	}
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			fprintf(fptr, "%d ", MatrixC[i][j]);
		}
	}
	fclose(fptr);
}

//function to compute 4% of the total rows for matrix C
void* PartialMatrixComputation(void* arg)
{
	//structure for nanosleep
	struct timespec tint;
	tint.tv_sec = 0;
	tint.tv_nsec = 10000;

	int index = (intptr_t)arg;
	int i, j, k = 0;

	//Computing matrix C
	for (i = (index*(size / 25)); i < ((index + 1) * (size / 25)); i++)
	{
		for (j = 0; j < size; j++)
		{
			for (k = 0; k < size; k++)
			{
				MatrixC[i][j] += MatrixA[i][k] * MatrixB[k][j];
			}
			if (nanosleep(&tint, NULL) < 0)
			{
				printf("nano sleep failed\n");
			}
		}
	}
}

int main()
{
	//for writing the start and end time
	int m1, m2 = 0;
	char timeBuf[26];
	struct tm *tm_info1, *tm_info2;
	struct timeval tv1, tv2;
	time_t start_t, end_t;
	double diff_t;

	int i, j = 0;

	//Generating random matrix A
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			MatrixA[i][j] = (rand() % 2001);
		}
	}

	//Generating random matrix B
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			MatrixB[i][j] = (rand() % 2001);
		}
	}

	//Writing Start Time
	gettimeofday(&tv1, NULL);
	m1 = (int)tv1.tv_usec;
	tm_info1 = localtime(&tv1.tv_sec);
	strftime(timeBuf, 26, "%Y-%m-%d %H-%M-%S", tm_info1);
	printf("Start Time: %s.%06d\n", timeBuf, m1);
	time(&start_t);

	//Creating 25 threads to compute matrix C
	for (i = 0; i < 25; i++)
	{
		pthread_create(&threadID[i], NULL, PartialMatrixComputation, (void*)(intptr_t)i);
	}

	//Waiting for all the threads to complete execution
	for (i = 0; i < 25; i++)
	{
		pthread_join(threadID[i], NULL);
	}

	//Writing End Time and execution time (in seconds)
	gettimeofday(&tv2, NULL);
	m2 = (int)tv2.tv_usec;
	tm_info2 = localtime(&tv2.tv_sec);
	strftime(timeBuf, 26, "%Y-%m-%d %H-%M-%S", tm_info2);
	printf("End Time: %s.%06d\n", timeBuf, m2);
	time(&end_t);
	diff_t = difftime(end_t, start_t);
	printf("Time to Execute in seconds: %f \n", diff_t);

	//write matrix C to the file MatrixCMT.txt
	WriteMatrixC();

	return 0;
}
