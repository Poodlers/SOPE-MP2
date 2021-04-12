#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "delay.h"

int unic_num = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


struct thread_param
{
	clock_t begin;
    int seconds_to_run;
    char* fifoname;
};

struct fifo_arg{
    int op;
    char* filename;
	clock_t begin;
	int seconds_to_run;
};

char ** get_info_from_fifo(const char *str, const char *delim)
{
    char *aux;
    char *p;
    char **res;
    char *argv[200]; /* place for 200 words. */
    int n = 0, i;

    assert((aux = strdup(str)));
    for (p = strtok(aux, delim); p; p = strtok(NULL, delim))
        argv[n++] = p;
    argv[n++] = NULL;
    /* i'll put de strdup()ed string one place past the NULL,
     * so you can free(3), once finished */
    argv[n++] = aux;
    /* now, we need to copy the array, so we can use it outside
     * this function. */
    assert((res = calloc(n, sizeof (char *))));
    for (i = 0; i < n; i++)
        res[i] = argv[i];
    return res;
}

void stdout_from_fifo(char* argv[],char * operation){
    fprintf(stdout,"%ld; ", time(0));
    for (int i = 0; argv[i]; i++)
        fprintf(stdout,"%s; ", argv[i]);
    fprintf(stdout,"%s \n", operation);
}

void read_from_fifo(char* private_fifo_name,clock_t begin, int time_out, char* client_request_args[5]){
	int np;
    
    while ((np = open(private_fifo_name, O_RDONLY)) < 0){
        continue;
    }

    char* buffer = "";
    int r;
    int seconds_elapsed = 0;
    do{
		r = read(np, buffer, 100);
        seconds_elapsed = (double)((clock() - begin) / sysconf(_SC_CLK_TCK));
        if(seconds_elapsed > time_out){
			stdout_from_fifo(client_request_args, "CLOSD");
			close(np);
            remove(private_fifo_name);
            return;
		}
	}while(r == 0);
    
    //argv has the arguments from the server's answer
    char ** server_answer_args = get_info_from_fifo(buffer," ");
    int res = atoi(server_answer_args[4]);

    if(res == -1){
        stdout_from_fifo(server_answer_args,"CLOSD");
    }
    else{
        stdout_from_fifo(server_answer_args,"GOTRS");
    }
    remove(private_fifo_name);
	close(np);
	return;
}

void* send_to_fifo(void* arg){
    struct fifo_arg *fifo_args = (struct fifo_arg *) arg;
	int np;
    char* argv[5];
    
    char* message = "";
	pthread_t id_thread = pthread_self();
	sprintf(message, "%d %d %ld %d\n", fifo_args->op, getpid(), id_thread,-1);

    char* private_fifo_name = "";
    sprintf(private_fifo_name, "/tmp/%d.%ld",getpid(), id_thread);
    
    if (mkfifo(private_fifo_name,0666) < 0) {
        perror("mkfifo");
    }

    pthread_mutex_lock(&mutex);
    while ((np = open(fifo_args->filename, O_WRONLY)) < 0){
        continue;
    }
	unic_num++;
    sprintf(argv[0], "%d", unic_num);
    char* i = "";
    sprintf(i,"%d ", unic_num);
    strcat(i,message);
    printf("%s \n", i);
	write(np, i,strlen(i));
    close(np);
	pthread_mutex_unlock(&mutex);

	sprintf(argv[1], "%d", fifo_args->op);
    sprintf(argv[2], "%d", getpid());
    sprintf(argv[3], "%ld",id_thread);
    argv[4] = "-1";

    stdout_from_fifo(argv,"IWANT");

    read_from_fifo(private_fifo_name,fifo_args->begin, fifo_args->seconds_to_run,argv);
	pthread_exit(NULL);
	 
}

void *thread_create(void* arg){
    struct thread_param *param = (struct thread_param *) arg;
    pthread_t ids[20];
    size_t num_of_threads = 0;
    struct fifo_arg fn;
    double seconds_elapsed = 0;
    
    fn.filename = param->fifoname;
	fn.begin = param->begin;
	fn.seconds_to_run = param->seconds_to_run;
	while(seconds_elapsed < param->seconds_to_run){
		fn.op = rand() % 9 + 1;
		pthread_create(&ids[num_of_threads], NULL, send_to_fifo, &fn);
        num_of_threads++;
        seconds_elapsed = (double)((clock() - param->begin) / sysconf(_SC_CLK_TCK));
		usleep(delay * 1000);
        
	}
    for(int i = 0; i< num_of_threads; i++) {
	    pthread_join(ids[i], NULL);	
	    
	}
    pthread_exit(NULL);
}


int main(int argc, char* argv[]){
    srand(time(NULL));
    clock_t begin = clock();
    int seconds_to_run = atoi(argv[1]);
    pthread_t c0;
    
    char * fifoname = "";
    strcpy(fifoname,"/tmp/");
    strcat(fifoname,argv[2]);
    
    if (mkfifo(fifoname,0666) < 0) {
        perror("mkfifo");
    }
    
    struct thread_param param;
    param.begin = begin;
    param.seconds_to_run = seconds_to_run;
    param.fifoname = fifoname;

    pthread_create(&c0, NULL, thread_create, &param);
    remove(fifoname);
    return 0;
}

