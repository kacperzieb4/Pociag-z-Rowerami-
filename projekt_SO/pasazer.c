#include "funkcje.h"
#include "dane.h"

int main() {
    setbuf(stdout, NULL);
    
    pid_t my_pid = getpid();  // PID bieżącego pasażera
    srand(my_pid);
    int has_bike = (rand() % 100 < 30) ? 1 : 0;  // Losowanie, czy pasażer ma rower

    if(has_bike == 1){
        printf("[PASAZER PID=%d] Pojawienie się na peronie z rowerem.\n", my_pid);
    }
    else
    {
        printf("[PASAZER PID=%d] Pojawienie się na peronie bez roweru.\n", my_pid);
    }

    struct message msg;
    msg.mtype = 1;  // Typ komunikatu oznaczający chęć wsiadania
    msg.ktype = my_pid;  // PID pasażera
    int q = get_message_queue(".", has_bike ? 1 : 0);  // Wybór kolejki w zależności od posiadania roweru
    if (q == -1) {
        perror("[PASAZER] Błąd pobrania kolejki");
        exit(1);
    }

    send_message(q, &msg);  // Wysyłanie zgłoszenia do kolejki

    pause();  // Czekanie na sygnał zakończenia
    return 0;
}