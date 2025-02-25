#include "funkcje.h"
#include "dane.h"

pid_t current_train_pid = -1;  // PID pociągu aktualnie na stacji

void sigusr1_handler_zawiadowca(int sig) {
    if (current_train_pid != -1) {
        kill(current_train_pid, SIGUSR1);  // Wysyłanie sygnału 1 do pociągu
        printf("\033[1;33m[ZAWIADOWCA] Wysłano sygnał 1 do pociągu PID=%d, aby wymusić odjazd.\033[0m\n", current_train_pid);
    }
}

void sigusr2_handler_zawiadowca(int sig) {
    if (current_train_pid != -1) {
        kill(current_train_pid, SIGUSR2);  // Wysyłanie sygnału 2 do pociągu
        printf("\033[1;33m[ZAWIADOWCA] Wysłano sygnał 2 do pociągu PID=%d, aby zablokować wsiadanie pasażerów.\033[0m\n", current_train_pid);
    }
}
void sigterm_handler(int sig) {
    printf("\033[1;33m[ZAWIADOWCA PID=%d] Otrzymano SIGTERM, kończenie procesu.\033[0m\n", getpid());
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGUSR1, sigusr1_handler_zawiadowca);  // Rejestracja sygnału 1
    signal(SIGUSR2, sigusr2_handler_zawiadowca);  // Rejestracja sygnału 2
    signal(SIGTERM, sigterm_handler); // Rejestracja sygnału końcowego 

    printf("\033[1;33m[ZAWIADOWCA] Start, PID=%d\033[0m\n", getpid());

    int platform_sem = sem_get(".", 5, 1);  // Semafor dla peronu
    int arriving_train_msq = get_message_queue(".", 2);  // Kolejka komunikatów dla przyjeżdżających pociągów
    int confirmation_msq = get_message_queue(".", 3);    // Kolejka komunikatów dla potwierdzeń

    struct message train_msg;

    while (1) {
        // Czekanie na zgłoszenie się pociągu
        if (receive_message(arriving_train_msq, 1, &train_msg) > 0) {
            
            long train_ID = train_msg.ktype;
            current_train_pid = train_ID;  // Zapis PID pociągu na stacji
            printf("\033[1;33m[ZAWIADOWCA] Pociąg PID=%ld zgłasza chęć wjazdu na stację.\033[0m\n", train_ID);

            // Zajęcie peronu
            sem_wait(platform_sem, 0);

            // Wysłanie potwierdzenie do pociągu
            printf("\033[1;33m[ZAWIADOWCA] Zezwalam pociągowi PID=%ld na wjazd na stację.\033[0m\n", train_ID);
            struct message confirmation_msg;
            confirmation_msg.mtype = train_ID;  // Użycie PIDu pociągu jako typu komunikatu
            confirmation_msg.ktype = 1;         // Potwierdzenie
            send_message(confirmation_msq, &confirmation_msg);
            

            // Czekanie na odjazd pociągu
            while (1) {
                if (receive_message_no_wait(arriving_train_msq, 2, &train_msg) > 0) {
                    printf("\033[1;33m[ZAWIADOWCA] Pociąg PID=%ld odjechał.\033[0m\n", train_ID);
                    current_train_pid = -1;  // Reset PIDu pociągu
                    break;
                }
                sleep(1);  // Symulacja oczekiwania na odjazd
            }

            // Zwolnienie peronu
            sem_raise(platform_sem, 0);
        }
    }
    return 0;
}