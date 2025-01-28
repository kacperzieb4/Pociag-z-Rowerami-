#include "funkcje.h"
#include "dane.h"

int force_departure = 0;  // Flaga wymuszająca odjazd pociągu
int block_passengers = 0;  // Flaga blokująca wsiadanie pasażerów
pid_t passenger_pids[P];  // Tablica przechowująca PIDy pasażerów w pociągu
int passenger_count = 0;  // Liczba pasażerów w pociągu

void sigusr1_handler_kierownik(int sig) {
    force_departure = 1;  // Ustawianie flagi wymuszonego odjazdu
    printf("[KIEROWNIK PID=%d] Otrzymano sygnał wymuszający odjazd.\n", getpid());
}

void sigusr2_handler_kierownik(int sig) {
    block_passengers = 1;  // Ustawianie flagi blokującej wsiadanie pasażerów
    printf("[KIEROWNIK PID=%d] Otrzymano sygnał blokujący wsiadanie pasażerów.\n", getpid());
}

void cleanup_passengers() {
    // Zabicie wszystkie procesy pasażerów w pociągu
    for (int i = 0; i < passenger_count; i++) {
        if (passenger_pids[i] > 0) {
            if (kill(passenger_pids[i], SIGKILL) == 0) {
                printf("[KIEROWNIK PID=%d] Pasażer PID=%d został zabity.\n", getpid(), passenger_pids[i]);
            } else {
                perror("kill");
            }
        }
    }
    passenger_count = 0;  // Reset liczby pasażerów
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGUSR1, sigusr1_handler_kierownik);  // Rejestracja sygnału 1
    signal(SIGUSR2, sigusr2_handler_kierownik);  // Rejestracja sygnału 2

    pid_t train_ID = getpid();  // PID bieżącego pociągu
    printf("[KIEROWNIK PID=%d] Start pociągu.\n", train_ID);

    int arriving_train_msq = get_message_queue(".", 2);  // Kolejka komunikatów dla przyjeżdżających pociągów
    int confirmation_msq = get_message_queue(".", 3);    // Kolejka komunikatów dla potwierdzeń

    // Dodatkowy semafor do blokowania wsiadania pasażerów podczas odjazdu
    int boarding_sem = sem_create(".", 7, 1);
    sem_set_value(boarding_sem, 0, 1);  // Początkowo wsiadanie jest dozwolone

    struct message train_msg;
    train_msg.mtype = 1;  // Typ komunikatu oznaczający chęć wjazdu na stację
    train_msg.ktype = train_ID;  // PID pociągu

    while (1) {
        // Powiadomienie zawiadowcy o chęci wjazdu na stację
        send_message(arriving_train_msq, &train_msg);
        printf("[KIEROWNIK PID=%d] Czekam na pozwolenie od zawiadowcy...\n", train_ID);

        // Czekanie na potwierdzenie od zawiadowcy
        struct message confirmation_msg;
        int received = 0;
        time_t start_wait_time = time(NULL);
        while (!received && (time(NULL) - start_wait_time < T)) {
            if (receive_message(confirmation_msq, train_ID, &confirmation_msg) == 1) {
                received = 1;
            } else {
                sleep(1);  // Czekanie na odpowiedź
            }
        }

        if (!received) {
            printf("[KIEROWNIK PID=%d] Nie otrzymano potwierdzenia od zawiadowcy w czasie %d sekund. Próba ponownie...\n", train_ID, T);
            continue;
        }

        printf("[KIEROWNIK PID=%d] **WJEŻDŻAM NA STACJĘ I OTWIERAM DRZWI**\n", train_ID);

        int pass_count = 0;  // Liczba pasażerów 
        int bike_count = 0;  // Liczba rowerów 
        struct message msg;
        time_t start_time = time(NULL);

        while ((pass_count < P && bike_count < R) && (time(NULL) - start_time < T || force_departure)) {
            if (force_departure) {
                printf("[KIEROWNIK PID=%d] Wymuszony odjazd! Przerywam wsiadanie pasażerów.\n", train_ID);
                break;  // Natychmiastowe przerwanie wsiadania pasażerów
            }

            if (block_passengers) {
                printf("[KIEROWNIK PID=%d] Wsiadanie pasażerów zablokowane.\n", train_ID);
                break;  // Natychmiastowe przerwanie wsiadania pasażerów
            }

            if (receive_message_no_wait(get_message_queue(".", 0), 1, &msg) == 1) {
                if (pass_count < P) {
                    printf("[KIEROWNIK] Pasażer PID=%ld zaczyna wsiadać (bez roweru).\n", msg.ktype);
                    sleep(2);
                    passenger_pids[passenger_count++] = msg.ktype;
                    pass_count++;
                    printf("[KIEROWNIK] Pasażer PID=%ld wsiadł (bez roweru).\n", msg.ktype);
                }
            }
            if (receive_message_no_wait(get_message_queue(".", 1), 1, &msg) == 1) {
                if (pass_count < P && bike_count < R) {
                    printf("[KIEROWNIK] Pasażer PID=%ld zaczyna wsiadać (z rowerem).\n", msg.ktype);
                    sleep(2);
                    passenger_pids[passenger_count++] = msg.ktype;
                    pass_count++;
                    bike_count++;
                    printf("[KIEROWNIK] Pasażer PID=%ld wsiadł (z rowerem).\n", msg.ktype);
                }
            }
            //sleep(1);
        }

        // Blokowanie wsiadania pasażerów przed odjazdem
        sem_wait(boarding_sem, 0);
        printf("[KIEROWNIK PID=%d] **ZAMYKAM DRZWI I ODJEŻDŻAM** (Liczba pasażerów: %d, rowerów: %d)\n", train_ID, pass_count, bike_count);

        // Powiadomienie zawiadowcy o odjeździe
        train_msg.mtype = 2;  // Typ komunikatu oznaczający odjazd
        send_message(arriving_train_msq, &train_msg);

        sleep(TI);  // Symulacja czasu jazdy pociągu
        printf("[KIEROWNIK PID=%d] **DOJECHAŁEM – WRACAM NA STACJĘ**\n", train_ID);

        // Oczyszczanie procesów pasażerów
        cleanup_passengers();

        sleep(TI);  // Symulacja czasu powrotu pociągu

        // Zwolnienie semafora po odjeździe
        sem_raise(boarding_sem, 0);

        force_departure = 0;  // Resetowanie flagi wymuszonego odjazdu
        block_passengers = 0; // Resetowanie flagi blokującej wsiadanie pasażerów

        // Ponownie zgłoszenie chęci wjazdu
        train_msg.mtype = 1;  // Typ komunikatu oznaczający chęć wjazdu
        send_message(arriving_train_msq, &train_msg);
        printf("[KIEROWNIK PID=%d] Czekam na pozwolenie od zawiadowcy...\n", train_ID);
    }
    return 0;
}