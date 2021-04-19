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
#include "common.h"

int unic_num = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


struct thread_param
{
	time_t begin;
    int seconds_to_run;
    char* fifoname;
};

struct fifo_arg{
    int op;
    char* filename;
	time_t begin;
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

void stdout_from_fifo(struct Message* msg,char * operation){
    fprintf(stdout,"%ld; ", time(NULL));
    fprintf(stdout,"%d; %d; %d; %ld; %d; ", msg->rid, msg->tskload, msg->pid, msg->tid,msg->tskres);
    fprintf(stdout,"%s \n", operation);
}

void read_from_fifo(char* private_fifo_name,time_t begin, int time_out, struct Message* request_msg ){
	int np;


	while ((np = open(private_fifo_name, O_RDONLY | O_NONBLOCK)) < 0){
        continue;
    }

    int r;
    int seconds_elapsed = 0;

	struct Message *msg = malloc(sizeof(struct Message));

	do{
		r = read(np, msg, sizeof(msg));
        seconds_elapsed = time(NULL) - begin;
        //the thread didn't receive an answer from the server in time
        if(seconds_elapsed > time_out){
			stdout_from_fifo(request_msg, "CLOSD");
			close(np);
            remove(private_fifo_name);
            return;
		}
	}while(r == 0);
    

	if(msg->tskres == -1){
        stdout_from_fifo(msg,"CLOSD");
    }
    else{
        stdout_from_fifo(msg,"GOTRS");
    }
	free(msg);
	close(np);
	return;
}

void* send_to_fifo(void* arg){
    struct fifo_arg *fifo_args = (struct fifo_arg *) arg;
	int np = -1;
	
	pthread_t id_thread = pthread_self();

    
    char* private_fifo_name = (char *) malloc(50 * sizeof(char));
    sprintf(private_fifo_name, "/tmp/%d.%ld",getpid(), id_thread);

	
    if (mkfifo(private_fifo_name,0666) < 0) {
        perror("mkfifo");
    }

	
    struct Message* msg = malloc(sizeof(struct Message));
    msg->tid = id_thread;
    msg->tskload = fifo_args->op;
    msg->pid = getpid();
    msg->tskres = -1;


	pthread_mutex_lock(&mutex);

    np = open(fifo_args->filename, O_WRONLY | O_NONBLOCK);


	unic_num++;

    msg->rid = unic_num;

	write(np, &msg, sizeof(msg));

	close(np);
	pthread_mutex_unlock(&mutex);
   
    stdout_from_fifo(msg,"IWANT");



    read_from_fifo(private_fifo_name,fifo_args->begin, fifo_args->seconds_to_run,msg);
    remove(private_fifo_name);
	free(private_fifo_name);
    free(msg);

	pthread_exit(NULL);
}

void *thread_create(void* arg){

    struct thread_param *param = (struct thread_param *) arg;
    pthread_t ids[100];
    size_t num_of_threads = 0;
    struct fifo_arg fn;
    double seconds_elapsed = 0;
    
    fn.filename = param->fifoname;
	fn.begin = param->begin;
	fn.seconds_to_run = param->seconds_to_run;
    
 

	while(seconds_elapsed < param->seconds_to_run){
		fn.op = rand() % 9 + 1; //carga da tarefa 
        
	    pthread_create(&ids[num_of_threads], NULL, send_to_fifo, &fn);
        num_of_threads++;
        seconds_elapsed = time(NULL) - param->begin;
        printf("seconds_elapsed: %f \n", seconds_elapsed);
		//usleep(delay * 1000);

        sleep(1);
	}

    for(int i = 0; i< num_of_threads; i++) {
	    pthread_join(ids[i], NULL);	
	
	}
    
    pthread_exit(NULL);
}


int main(int argc, char* argv[]){
   
    time_t begin = time(NULL);
    srand(time(NULL));
    
    int seconds_to_run = atoi(argv[1]);
    
    pthread_t c0;
    
    
    char* fifoname = (char *) malloc(50 * sizeof(char));
 
    
    sprintf(fifoname,"/tmp/%s",argv[2]);


    if (mkfifo(fifoname,0666) < 0) {
        perror("mkfifo");
    }
 
     
    struct thread_param param;
    param.begin = begin;
    param.seconds_to_run = seconds_to_run;
    param.fifoname = fifoname;
    
    
    pthread_create(&c0, NULL, thread_create, &param);
    
    pthread_join(c0,NULL);

 
    remove(fifoname);
    
	free(fifoname);

	return 0;
    
}

