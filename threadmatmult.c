/**
 * compile: 
 * gcc -pthread -std=c99 -o test threadmatmult_14.c && time ./test 600 500 700 50  //x=600, y=500, z=700, n=50
 * gcc -pthread -std=c99 -o test threadmatmult_14.c && time ./test x y z n 
 * 
 * matrix multiplication: matC = matA x matB
 * x,y,z... matrix dimensions: matA[x][y], matB[y][z], matC[x][z]
 * n...number of threads
 */

# include <stdlib.h>
# include <stdio.h>
# include <pthread.h>
# include <time.h>

void mutex_init(pthread_mutex_t *mutex) {
	if (pthread_mutex_init(mutex, NULL) != 0) {
        printf("pthread_mutex_init error");
		exit(EXIT_FAILURE);
    }
}


void mutex_lock(pthread_mutex_t *mutex){
	if (pthread_mutex_lock(mutex) != 0) {
		perror("pthread_mutex_lock error");
		exit(EXIT_FAILURE);
	}
}

void mutex_unlock(pthread_mutex_t *mutex) {
	if (pthread_mutex_unlock(mutex) != 0) {
		perror("pthread_mutex_unlock error");
		exit(EXIT_FAILURE);
	}
}

typedef struct {
	int x;
	int y;
	int z;
	int n;
	int **matA;
	int **matB;
	int **matC;
	int *thread_array; //includes, how many lines of data.matA each thread has to calculate
	int line;
	int thread_nr;
	pthread_mutex_t mutex;
} data_struct;

data_struct data;

/* traegt die argumente x,y,z,n in das struct data ein und initialisiert data.line & data.thread_nr mit 0 */
void arg_insert(int x, int y, int z, int n){
	data.x = x;
	data.y = y;
	data.z = z;
	data.n = n;
	data.line = 0;
	data.thread_nr = 0;
	mutex_init(&data.mutex);
}

/* matritzen data.matA, data.matB und data.matC werden mit malloc/calloc definiert. A und B mit zufallszahlen befuellt (C mit calloc - "0") */
void mat_alloc_init(){
//	srand(time(NULL));
	
	if ((data.matA = malloc(sizeof(int*) * data.x)) == NULL) {
		perror("malloc error");
		exit(EXIT_FAILURE);
	}
	for(int x = 0; x < data.x; x++) {
		if ((data.matA[x] = malloc(sizeof(int) * data.y)) == NULL) {
			perror("malloc error");
			exit(EXIT_FAILURE);
		}
	}
	for (int x = 0; x < data.x; x++) {
		for (int y = 0; y < data.y; y++) {
			data.matA[x][y] = rand() %10;
		}
	}

	if ((data.matB = malloc(sizeof(int*) * data.y)) == NULL) {
		perror("malloc error");
		exit(EXIT_FAILURE);
	}
	for (int y = 0; y < data.y; y++) {
		if ((data.matB[y] = malloc(sizeof(int) * data.z)) == NULL) { //korr: data.y -> data.z
			perror("malloc error");
			exit(EXIT_FAILURE);
		}
	}
	for (int y = 0; y < data.y; y++) {
		for (int z = 0; z < data.z; z++) {
			data.matB[y][z] = rand() %10;
		}
	}

	if ((data.matC = calloc(data.x, sizeof(int*))) == NULL) {
		perror("calloc error");
		exit(EXIT_FAILURE);
	}
	for (int x = 0; x < data.x; x++) {
		if ((data.matC[x] = calloc(data.z, sizeof(int))) == NULL) {
			perror("calloc error");
			exit(EXIT_FAILURE);
		}
	}
}

/* free of malloc/calloc - data.matA, data.matB, data.matC */
void free_mat() {
	for (int x = 0; x < data.x; x++) {
		free(data.matA[x]);
		free(data.matC[x]);
	}
	for (int y = 0; y < data.y; y++) {
		free(data.matB[y]);
	}
	free(data.matA);
	free(data.matB);
	free(data.matC);
}

/* printf von data.matA, data.matB, data.matC */
void print_mat() {
	printf("\ndata.matA:\n");
	for (int x = 0; x < data.x; x++) {
		for (int y = 0; y < data.y; y++) {
			printf("%d\t", data.matA[x][y]);
		}
		printf("\n");
	}
	
	printf("\n\ndata.matB:\n");
	for (int y = 0; y < data.y; y++) {
		for (int z = 0; z < data.z; z++) {
			printf("%d\t", data.matB[y][z]);
		}
		printf("\n");
	}
	
	printf("\n\ndata.matC:\n");	
	for (int x = 0; x < data.x; x++) {
		for (int z = 0; z < data.z; z++) {
			printf("%d\t", data.matC[x][z]);
		}
		printf("\n");
	}
}

/* calculates, how many lines of data.matA each thread has to calculate -> the result is written in in the array: thread_arr where the pointer data.thread_arrays (struct) points to */
void thread_lines(int thread_arr[data.n]) {
	for (int i = 0; i < data.n; i++) {
		data.thread_array = thread_arr;
		thread_arr[i] = (i<(data.x % data.n)) ? ((data.y/data.n)+1) : (data.y/data.n);
//		printf("thread <%d> = %d lines\n", i, data.thread_array[i]);
	}
}

/* matrix multiplication for the appropriate line of data.matA */
void mat_mult (int line) {
	for (int z = 0; z < data.z; z++) {
		for (int y = 0; y < data.y; y++) {
			data.matC[line][z] += data.matA[line][y] * data.matB[y][z];
		}
	} 
}

/* in data.thread_array are the numbers of lines (of data.matA) to be calated for each thread */
void *thread_function() {
//	printf("data.thread_array[data.thread_nr]: %d\tdata.thread_nr: %d\n", data.thread_array[data.thread_nr], data.thread_nr);
	mutex_lock(& data.mutex);
	while (data.thread_array[data.thread_nr] != 0) {
//		printf("in while: \tdata.thread_array[data.thread_nr]: %d\tdata.thread_nr: %d\n", data.thread_array[data.thread_nr], data.thread_nr);
		mat_mult(data.line);
		data.line++;
		data.thread_array[data.thread_nr]--;
	}
	data.thread_nr++;
	mutex_unlock(& data.mutex);
	pthread_exit((void *)pthread_self());	
}


int main (int argc, char **argv) {
	if (argc != 5) {
		printf("arguments: <./program> <x_dim_of_matrix> <y_dim_of_matrix> <z_dim_of_matrix> <number_of_threads>");
		exit(EXIT_FAILURE); 
	}
	
	arg_insert(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	mat_alloc_init();
	
	int thread_arr[data.n];
	thread_lines(thread_arr);
	
	pthread_t pid[data.n];

	//take start-time stamp
	clock_t start = clock();

	for (int i = 0; i < data.n; i++) {
		if (pthread_create(&(pid[i]), NULL, thread_function, NULL) != 0) {
				perror("pthread_create error");
				exit(EXIT_FAILURE);
		}
	}
	
	for (int i = 0; i < data.n; i++) {
		if ((pthread_join(pid[i], NULL)) != 0 ) {
			perror("pthread_join error");
			exit(EXIT_FAILURE);
		}
	}
	
	//take end-time stamp
	clock_t stop = clock();
	
	double time = (stop - start)/1000;

//	print_mat();	
	printf("time:    %f ms\n", time);	
	free_mat();
	
	printf("x: %d, y: %d, z: %d, n: %d\n", data.x, data.y, data.z, data.n);
	return (EXIT_SUCCESS);
}
# pthread

