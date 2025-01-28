#include "funkcje.h"
#include "dane.h"

pid_t current_train_pid = -1;  // PID pociągu aktualnie na stacji

void sigusr1_handler_zawiadowca(int sig) {
    if (current_train_pid != -1) {
        kill(current_train_pid, SIGUSR1);  // Wysyłanie sygnału 1 do pociągu
        printf("[ZAWIADOWCA] Wysłano sygnał 1 do pociągu PID=%d, aby wymusić odjazd.\n", current_train_pid);
    }
}

void sigusr2_handler_zawiadowca(int sig) {
    if (current_train_pid != -1) {
        kill(current_train_pid, SIGUSR2);  // Wysyłanie sygnału 2 do pociągu
        printf("[ZAWIADOWCA] Wysłano sygnał 2 do pociągu PID=%d, aby zablokować wsiadanie pasażerów.\n", current_train_pid);
    }
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGUSR1, sigusr1_handler_zawiadowca);  // Rejestracja sygnału 1
    signal(SIGUSR2, sigusr2_handler_zawiadowca);  // Rejestracja sygnału 2

    printf("[ZAWIADOWCA] Start, PID=%d\n", getpid());

    int platform_sem = sem_get(".", 5, 1);  // Semafor dla peronu
    int arriving_train_msq = get_message_queue(".", 2);  // Kolejka komunikatów dla przyjeżdżających pociągów
    int confirmation_msq = get_message_queue(".", 3);    // Kolejka komunikatów dla potwierdzeń

    struct message train_msg;

    while (1) {
        // Czekanie na zgłoszenie się pociągu
        if (receive_message(arriving_train_msq, 1, &train_msg) > 0) {
            long train_ID = train_msg.ktype;
            current_train_pid = train_ID;  // Zapis PID pociągu na stacji
            printf("[ZAWIADOWCA] Pociąg PID=%ld zgłasza chęć wjazdu na stację.\n", train_ID);

            // Zajęcie peronu
            sem_wait(platform_sem, 0);

            // Wyśłanie potwierdzenie do pociągu
            struct message confirmation_msg;
            confirmation_msg.mtype = train_ID;  // Użycie PIDu pociągu jako typu komunikatu
            confirmation_msg.ktype = 1;         // Potwierdzenie
            send_message(confirmation_msq, &confirmation_msg);

            printf("[ZAWIADOWCA] Zezwalam pociągowi PID=%ld na wjazd na stację.\n", train_ID);

            // Czekanie na odjazd pociągu
            while (1) {
                if (receive_message_no_wait(arriving_train_msq, 2, &train_msg) > 0) {
                    printf("[ZAWIADOWCA] Pociąg PID=%ld odjechał.\n", train_ID);
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