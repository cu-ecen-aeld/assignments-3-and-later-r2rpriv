#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data *thrdata = (struct thread_data*)thread_param;
    usleep(thrdata->wto_ms * 1000);
    pthread_mutex_lock(thrdata->mtx);
    usleep(thrdata->wtr_ms * 1000);
    pthread_mutex_unlock(thrdata->mtx);
    thrdata->thread_complete_success =  true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *thread_args = (struct thread_data *) malloc(sizeof(struct thread_data));
    thread_args->wto_ms = wait_to_obtain_ms;
    thread_args->wtr_ms = wait_to_release_ms;
    thread_args->mtx = mutex;

    if( pthread_create(thread, NULL, &threadfunc,(void *)thread_args) !=0 )
    	return false;
    else
	return true;
}

