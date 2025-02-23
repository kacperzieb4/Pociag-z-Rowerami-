#include "funkcje.h"
#include "dane.h"

void* start_kierownikow(void* arg) {
    int created_trains = 0;
    for (int i = 0; i < N; i++) {
        pid_t kid = fork();
        if (kid == -1) {
            perror("fork kierownik");
            continue;
        } else if (kid == 0) {
            execl("./kierownik", "kierownik", NULL);
            perror("execl kierownik");
            exit(1);
        }
        created_trains++;
    }
    if (created_trains != N) {
        printf("\033[1;31m[MASTER] Liczba utworzonych procesów kierowników (%d) nie zgadza się z oczekiwaną wartością N (%d).\033[0m\n", created_trains, N);
    }
    return NULL;
}

void* start_pasazerowie(void* arg) {
    int created_passengers = 0;
    for (int i = 0; i < TP; i++) {
        pid_t pid_p = fork();
        if (pid_p == -1) {
            perror("fork pasazer");
            continue;
        } else if (pid_p == 0) {
            execl("./pasazer", "pasazer", NULL);
            perror("execl pasazer");
            exit(1);
        }
        created_passengers++;
        usleep((rand() % 2000 + 500) * 1000);
    }
    if (created_passengers != TP) {
        printf("\033[1;31m[MASTER] Liczba utworzonych procesów pasażerów (%d) nie zgadza się z oczekiwaną wartością TP (%d).\033[0m\n", created_passengers, TP);
    }
    return NULL;
}

// Inicjalizacja pamięci dla liczby zabitych pasażerów
int killed_passengers = 0;

void* cleanup_passengers_main(void* arg) {
    int cleanup_msq = get_message_queue(".", 4);
    struct message msg;

    while (killed_passengers < TP) {
        if (receive_message(cleanup_msq, 1, &msg) == 1) {
            pid_t passenger_pid = msg.ktype;
            if (kill(passenger_pid, SIGKILL) == 0) {
                printf("\033[;35mPasażer PID=%d dojechał do celu.\033[0m\n", passenger_pid);
                killed_passengers++;
                waitpid(passenger_pid, NULL, 0);  // Zapobieganie zombie
            }
        }
        usleep(1000);
    }
    return NULL;
}

void cleanup() {
    printf("\033[1;34m[MASTER] Sprzątanie zasobów.\033[0m\n");

    system("pkill -SIGTERM kierownik");  // Wysłanie sygnału zakończenia do kierowników
    system("pkill -SIGTERM zawiadowca"); // Wysłanie sygnału zakończenia do zawiadowcy
    
    // Usuwanie kolejek komunikatów
    destroy_message_queue(get_message_queue(".", 0));
    destroy_message_queue(get_message_queue(".", 1));
    destroy_message_queue(get_message_queue(".", 2));
    destroy_message_queue(get_message_queue(".", 3));
    destroy_message_queue(get_message_queue(".", 4));

    // Usuwanie semaforów
    sem_destroy(sem_get(".", 5, 1)); // Peron
    sem_destroy(sem_get(".", 6, 1)); // Synchronizacja pociągów

    printf("\033[1;34m[MASTER] Wszystkie zasoby zostały usunięte.\033[0m\n");
    exit(0);
}

void sigint_handler_main(int sig) {
    printf("\n\033[1;34m[MASTER] SIGINT \033[0m\n");
    cleanup();
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGINT, sigint_handler_main);  // Rejestracja sygnału SIGINT
    printf("\033[1;34m[MASTER] Start programu. Tworzenie procesów.\033[0m\n");

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
    if (zawiadowca_pid == -1) {
        perror("fork zawiadowca");
        exit(1);
    } else if (zawiadowca_pid == 0) {
        execl("./zawiadowca", "zawiadowca", NULL);
        perror("execl zawiadowca");
        exit(1);
    }

    sleep(1);  // Czekanie, aby zawiadowca się uruchomił
    pthread_t kierownicy_thread, pasazerowie_thread, cleanup_thread;
    // Tworzenie wątków do równoczesnego uruchomienia procesów
    if (pthread_create(&kierownicy_thread, NULL, start_kierownikow, NULL) != 0) {
        perror("pthread_create kierownicy_thread");
        exit(1);
    }
    if (pthread_create(&pasazerowie_thread, NULL, start_pasazerowie, NULL) != 0) {
        perror("pthread_create pasazerowie_thread");
        exit(1);
    }
    if (pthread_create(&cleanup_thread, NULL, cleanup_passengers_main, NULL) != 0) {
        perror("pthread_create cleanup_thread");
        exit(1);
    }

    // Czekanie na zakończenie wątków
    if (pthread_join(kierownicy_thread, NULL) != 0) {
        perror("pthread_join kierownicy_thread");
    }
    if (pthread_join(pasazerowie_thread, NULL) != 0) {
        perror("pthread_join pasazerowie_thread");
    }

    // Monitorowanie liczby zabitych(rozwiezionych) pasażerów
    while (killed_passengers < TP) {
        usleep(10000);
    }
    if (pthread_join(cleanup_thread, NULL) != 0) {
        perror("pthread_join cleanup_thread");
    }
    printf("\033[1;34m[MASTER] Wszyscy pasazerowie dojechali do stacji końcowej. Kończę program.\033[0m\n");
    cleanup();
}