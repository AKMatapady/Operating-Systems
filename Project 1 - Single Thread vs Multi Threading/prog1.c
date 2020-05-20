#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

int i, j, k = 0;
#define size 10000
int MatrixA[size][size], MatrixB[size][size], MatrixC[size][size];

//function to write the resultant matrix C
void WriteMatrixC()
{
	FILE *fptr;
	int i, j = 0;
	if ((fptr = fopen("MatrixCST.txt", "w")) == NULL)
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

int main()
{
	//structure for nanosleep
	struct timespec tint;
	tint.tv_sec = 0;
	tint.tv_nsec = 10000;		//10 micro seconds

	//for writing the start and end time
	int m1, m2 = 0;
	char timeBuf[26];
	struct tm *tm_info1, *tm_info2;
	struct timeval tv1, tv2;
	time_t start_t, end_t;
	double diff_t;

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

	//Computing matrix C
	for (i = 0; i < size; i++)
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
			//usleep(10);
		}
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

	//write matrix C to the file MatrixCST.txt
	WriteMatrixC();
	return 0;
}
