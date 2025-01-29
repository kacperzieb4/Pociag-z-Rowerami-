#include "funkcje.h"

//Kolejki komunikatow
int get_message_queue(const char *path, int proj_ID) {
    key_t key = ftok(path, proj_ID);
    if (key == -1) {
        perror("ftok get_message_queue");
        return -1;
    }
    int msq_ID = msgget(key, IPC_CREAT | 0600);
    if (msq_ID == -1) {
        perror("msgget get_message_queue");
        return -1;
    }
    return msq_ID;
}

int send_message(int msq_ID, struct message *msg) {
    if (msgsnd(msq_ID, msg, sizeof(*msg) - sizeof(long), 0) == -1) {
        perror("msgsnd send_message");
        return -1;
    }
    return 0;
}

void destroy_message_queue(int msq_ID) {
    if (msgctl(msq_ID, IPC_RMID, NULL) == -1) {
        perror("msgctl destroy_message_queue");
    }
}

int receive_message(int msq_ID, long msgtype, struct message *msg) {
    if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, 0) == -1) {
        if (errno == ENOMSG) {
            return 0;
        } else {
            perror("msgrcv receive_message");
            return -1;
        }
    }
    return 1;
}

int receive_message_no_wait(int msq_ID, long msgtype, struct message *msg) {
    if (msgrcv(msq_ID, msg, sizeof(*msg) - sizeof(long), msgtype, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
            return 0;
        } else {
            perror("msgrcv receive_message_no_wait");
            return -1;
        }
    }
    return 1;
}

//Semafory
int sem_create(char* unique_path, int project_name, int nsems) {
    key_t key = ftok(unique_path, project_name);
    if (key == -1) {
        perror("ftok sem_create");
        exit(1);
    }
    int sem_ID = semget(key, nsems, 0666 | IPC_CREAT);
    if (sem_ID == -1) {
        perror("semget sem_create");
        exit(1);
    }
    return sem_ID;
}

int sem_get(char* unique_path, int project_name, int nsems) {
    key_t key = ftok(unique_path, project_name);
    if (key == -1) {
        perror("ftok sem_get");
        exit(1);
    }
    int sem_ID = semget(key, nsems, 0666);
    if (sem_ID == -1) {
        perror("semget sem_get");
        exit(1);
    }
    return sem_ID;
}

void sem_set_value(int sem_ID, int num, int value) {
    if (semctl(sem_ID, num, SETVAL, value) == -1) {
        perror("semctl sem_set_value");
        exit(1);
    }
}

void sem_wait(int sem_ID, int num) {
    struct sembuf sops = { num, -1, 0 };
    while (1) {
        if (semop(sem_ID, &sops, 1) == -1) {
            if (errno == EINTR) continue;
            perror("semop sem_wait");
            exit(1);
        }
        break;
    }
}

void sem_raise(int sem_ID, int num) {
    struct sembuf sops = { num, 1, 0 };
    if (semop(sem_ID, &sops, 1) == -1) {
        perror("semop sem_raise");
        exit(1);
    }
}


void sem_destroy(int sem_ID) {
    if (semctl(sem_ID, 0, IPC_RMID, NULL) == -1) {
        perror("semctl sem_destroy");
        exit(1);
    }
}

//Pamiec wspoldzielona
int shared_mem_create(char* unique_path, int project_name, size_t size) {
    key_t key = ftok(unique_path, project_name);
    if (key == -1) {
        perror("ftok shared_mem_create");
        exit(1);
    }
    int mem_ID = shmget(key, size, 0666 | IPC_CREAT);
    if (mem_ID == -1) {
        perror("shmget shared_mem_create");
        exit(1);
    }
    return mem_ID;
}

int shared_mem_get(char* unique_path, int project_name) {
    key_t key = ftok(unique_path, project_name);
    if (key == -1) {
        perror("ftok shared_mem_get");
        exit(1);
    }
    int mem_ID = shmget(key, 0, 0666);
    if (mem_ID == -1) {
        perror("shmget shared_mem_get");
        exit(1);
    }
    return mem_ID;
}

int* shared_mem_attach_int(int mem_ID) {
    int* ptr = (int*)shmat(mem_ID, NULL, 0);
    if (ptr == (int*)-1) {
        perror("shmat shared_mem_attach_int");
        exit(1);
    }
    return ptr;
}

void shared_mem_destroy(int mem_ID) {
    if (shmctl(mem_ID, IPC_RMID, NULL) == -1) {
        perror("shmctl shared_mem_destroy");
        exit(1);
    }
}


