#include "funkcje.h"
#include "dane.h"

void sigint_handler_main(int sig) {
    printf("\n[MASTER] SIGINT - kończę i czyszczę zasoby.\n");

    // Usuwanie kolejek komunikatów
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    destroy_message_queue(get_message_queue(".", 2));
    destroy_message_queue(get_message_queue(".", 3));

    // Usuwanie semaforów
    sem_destroy(sem_get(".", 5, 1)); // Peron
    sem_destroy(sem_get(".", 6, 1)); // Synchronizacja pociągów

    printf("[MASTER] Wszystkie zasoby zostały usunięte.\n");
    exit(0);
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGINT, sigint_handler_main);  // Rejestracja sygnału SIGINT
    
    printf("[MASTER] Start programu. Tworzenie procesów...\n");

    // Inicjalizacja semaforów
    int platform_sem = sem_create(".", 5, 1);  // Semafor peronu
    if (platform_sem == -1) {
        perror("Błąd podczas tworzenia semafora platform_sem");
        exit(1);
    }
    sem_set_value(platform_sem, 0, 1);  // Peron początkowo wolny (wartość 1)

    int next_train_sem = sem_create(".", 6, 1);  // Semafor do synchronizacji pociągów
    if (next_train_sem == -1) {
        perror("Błąd podczas tworzenia semafora next_train_sem");
        exit(1);
    }
    sem_set_value(next_train_sem, 0, 1);  // Następny pociąg może wjechać (wartość 1)

    // Uruchomienie zawiadowcy
    pid_t zawiadowca_pid = fork();
    if (zawiadowca_pid == 0) {
        execl("./zawiadowca", "zawiadowca", NULL);
        perror("execl zawiadowca");
        exit(1);
    }

    sleep(1);  // Czekanie, aby zawiadowca się uruchomił

    // Uruchomienie procesów kierowników
    for (int i = 0; i < N; i++) {
        pid_t kid = fork();
        if (kid == 0) {
            execl("./kierownik", "kierownik", NULL);
            perror("execl kierownik");
            exit(1);
        }
        sleep(1); 
    }
    // Uruchomienie procesów pasażerów
    for (int i = 0; i < TP; i++) {
        pid_t pid_p = fork();
        if (pid_p == 0) {
            printf("%d",i+1);
            execl("./pasazer", "pasazer", NULL);
            perror("execl pasazer");
            exit(1);
        }
        usleep((rand() % 2000 + 500) * 1000);  // Losowe opóźnienie między pasażerami
    }
    
    while (wait(NULL) > 0);  // Czekanie na zakończenie wszystkich procesów

    sigint_handler_main(SIGINT);
    return 0;
}