////////////////////////////////////////////////////////////////////////////////////////////////

Large Scale Matrix Multiplication
----------------------------------

This project is a part of the course CS5348.001 Operating System Concepts offered during Fall 2018 at UTD.

The problem statement is to write two programs, prog1.c and prog2.c, that multiply two large matrices A and B of 10000 rows and columns (square matrices) and generate a matrix C. prog1.c is a single threaded program while prog2.c is a multi-threaded program with 25 threads running at the same time to generate Matrix C.

Both these programs add a delay of about 10 micro-seconds for generating each element in matrix C. We have defined a constant 'a', which we use to vary the size of square matrices and check the output, in both the prog1.c and prog2.c. We are writing the 'Start Time' and 'End Time' for the matrix C computation within the two programs.


Files
----------------------------------

Output10000.txt 
Contains the sequence of commands and the respective outputs observed while executing the above programs for a=10000, i.e. for Matrix A and B of 10000 * 10000.

Output500.txt
Contains the sequence of commands and the respective outputs observed while executing the above programs for a=500, i.e. for Matrix A and B of 500 * 500.

prog1.c
Contains the single threaded program to compute Matrix multiplication. The resultant matrix C is written to MatrixCST.txt.

prog2.c
Contains the multi-threaded program to compute Matrix multiplication. The resultant matrix C is written to MatrixCMT.txt.

MatrixCMT.txt
Contains the result of matrix multiplication of random matrices A and B of 500 rows and columns done using multi-threading.

MatrixCST.txt
Contains the result of matrix multiplication of random matrices A and B of 500 rows and columns done using single-threading.

Participants
--------------------------------------
axm180029 - Amith Kumar Matapady
sxb180020 - Swapnil Bansal
sxb180026 - Surajit Baitalik


Comments
--------------------------------------

The rand() function generates the same set of pseudo-random numbers unless preceded with an 'srand(an unique value)'. We have used this feature of rand() to ensure that both prog1.c and prog2.c have the same matrix A and B for matrix multiplication.

We have run these programs on the UTD servers (cs2.utdallas.edu and csgrads1.utdallas.edu). These servers have a memory space limit of about 1GB. The output files (MatrixCMT.txt and MatrixCST.txt) for 10000 rows and columns was coming upto 1GB each. So we couldn't compare these files on the UTD servers, which is why we used 500 rows and columns to compare and verify that both the programs have the same resultant matrix C. The steps for the matrices with 500 rows and columns are in the file Output500.txt.

We have also run the programs for 10000 rows and columns and recorded the time taken by both these programs. The steps for these are in the file Output10000.txt.

Also for the 10 micro-second sleep, we have tested out usleep() and nanosleep() APIs. The APIs available for (micro-second or lower) sleep only guarantee that the program sleeps for a MINIMUM of the specified time and NOT EXACTLY the specified time. We found nanosleep() to be more accurate than usleep during our test runs. Hence we have used nanosleep().

/////////////////////////////////////////////////////////////////////////////////////////////////