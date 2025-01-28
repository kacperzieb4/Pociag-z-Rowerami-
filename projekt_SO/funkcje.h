#ifndef MOJE_FUNKCJE_H
#define MOJE_FUNKCJE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define MSG_MAX 256

// Struktura komunikatu w kolejce
struct message {
    long mtype;  // typ
    long ktype;  // np. PID, inny identyfikator
};

//Kolejki komunikatow
int  get_message_queue(const char *path, int proj_ID);
int  send_message(int msq_ID, struct message *msg);
void destroy_message_queue(int msq_ID);
int  receive_message(int msq_ID, long msgtype, struct message *msg);
int  receive_message_no_wait(int msq_ID, long msgtype, struct message *msg);

//Semafory
int  sem_create(char* unique_path, int project_name, int nsems);
int  sem_create_once(char* unique_path, int project_name, int nsems);
int  sem_get(char* unique_path, int project_name, int nsems);
int  sem_get_return(char* unique_path, int project_name, int nsems);
void sem_set_value(int sem_ID, int num, int value);
void sem_wait(int sem_ID, int num);
int  sem_wait_interruptible(int sem_ID, int num);
void sem_raise(int sem_ID, int num);
void sem_lower_no_wait(int sem_ID, int num);
int  sem_exists(char* unique_path, int project_name, int nsems);
void sem_destroy(int sem_ID);

//Pamiec wspoldzielona
int   shared_mem_create(char* unique_path, int project_name, size_t size);
int   shared_mem_get(char* unique_path, int project_name);
int   shared_mem_size(int mem_ID);
int   shared_mem_get_return(char* unique_path, int project_name);
char* shared_mem_attach_char(int mem_ID);
int*  shared_mem_attach_int(int mem_ID);
void  shared_mem_detach(char *shared_mem);
void  shared_mem_destroy(int mem_ID);

#endif