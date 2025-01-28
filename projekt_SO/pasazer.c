#include "funkcje.h"
#include "dane.h"

void sigusr2_handler(int sig) {
    printf("[PASAZER PID=%d] Otrzymano sygnał 2, nie mogę wejść na peron.\n", getpid());
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGUSR2, sigusr2_handler);  // Rejestracja sygnału 2
    
    pid_t my_pid = getpid();  // PID bieżącego pasażera
    srand(my_pid);
    int has_bike = (rand() % 100 < 30) ? 1 : 0;  // Losowanie, czy pasażer ma rower

    printf("[PASAZER PID=%d] Pojawienie się na peronie (rower=%d).\n", my_pid, has_bike);

    struct message msg;
    msg.mtype = 1;  // Typ komunikatu oznaczający chęć wsiadania
    msg.ktype = my_pid;  // PID pasażera
    int q = get_message_queue(".", has_bike ? 1 : 0);  // Wybór kolejki w zależności od posiadania roweru
    if (q == -1) {
        perror("[PASAZER] Błąd pobrania kolejki");
        exit(1);
    }

    // Sprawdzenie, czy wsiadanie jest zablokowane
    if (kill(getppid(), 0) == -1) {
        printf("[PASAZER PID=%d] Wsiadanie jest zablokowane, nie mogę wejść na peron.\n", my_pid);
        exit(0);
    }

    send_message(q, &msg);  // Wysyłanie zgłoszenia do kolejki

    pause();  // Czekanie na sygnał zakończenia
    return 0;
}