#include "funkcje.h"
#include "dane.h"

void cleanup() {
    printf("\n[MASTER] Sprzątanie zasobów.\n");

    system("pkill -SIGTERM kierownik");  // Wysłanie sygnału zakończenia do kierowników
    system("pkill -SIGTERM zawiadowca"); // Wysłanie sygnału zakończenia do zawiadowcy
    
    // Usuwanie kolejek komunikatów
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    destroy_message_queue(get_message_queue(".", 2));
    destroy_message_queue(get_message_queue(".", 3));

    // Usuwanie semaforów
    sem_destroy(sem_get(".", 5, 1)); // Peron
    sem_destroy(sem_get(".", 6, 1)); // Synchronizacja pociągów

    //Usuwanie pamięci współdzielonej
    shared_mem_destroy(shared_mem_get(".", 7));

    printf("[MASTER] Wszystkie zasoby zostały usunięte.\n");
    exit(0);
}

void sigint_handler_main(int sig) {
    printf("\n[MASTER] SIGINT");
    cleanup();
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
    
    // Inicjalizacja pamięci współdzielonej dla liczby zabitych pasażerów
    int shm_id = shared_mem_create(".", 7, sizeof(int));
    int *killed_passengers = shared_mem_attach_int(shm_id);
    *killed_passengers = 0;  // Inicjalizacja licznika
    printf("%d",shared_mem_get(".", 7));


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
    
    sleep(1);

    // Uruchomienie procesów pasażerów
    for (int i = 0; i < TP; i++) {
        pid_t pid_p = fork();
        if (pid_p == 0) {
            execl("./pasazer", "pasazer", NULL);
            perror("execl pasazer");
            exit(1);
        }
        usleep((rand() % 2000 + 500) * 1000);  // Losowe opóźnienie między pasażerami
    }
    
    // Monitorowanie liczby zabitych pasażerów
    while (*killed_passengers < TP) {
        sleep(1);
    }
    printf("[MASTER] Wszyscy pasazerowie dojechali do stacji końcowej. Kończę program.\n");
    cleanup();
}