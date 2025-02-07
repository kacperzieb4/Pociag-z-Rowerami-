#include "funkcje.h"
#include "dane.h"

int force_departure = 0;  // Flaga wymuszająca odjazd pociągu
int block_passengers = 0;  // Flaga blokująca wsiadanie pasażerów
pid_t passenger_pids[P];  // Tablica przechowująca PIDy pasażerów w pociągu
int passenger_count = 0;  // Liczba pasażerów w pociągu
int boarding_in_progress = 0; // Flaga informująca o trwającym wsiadaniu


void sigusr1_handler_kierownik(int sig) {
    force_departure = 1;  // Ustawienie flagi wymuszonego odjazdu
    printf("\033[1;32m[KIEROWNIK PID=%d] Otrzymano sygnał wymuszający odjazd.\033[0m\n", getpid());
}

void sigusr2_handler_kierownik(int sig) {
    block_passengers = 1; // Ustawienie flagi blokującej wsiadanie pasażerów
    printf("\033[1;32m[KIEROWNIK PID=%d] Otrzymano sygnał blokujący wsiadanie pasażerów.\033[0m\n", getpid());

}

void sigterm_handler(int sig) {
    printf("\033[1;32m[KIEROWNIK PID=%d] Otrzymano SIGTERM, kończenie procesu.\033[0m\n", getpid());
    exit(0);
}

void cleanup_passengers() {
    int shm_id = shared_mem_get(".", 7);
    int *killed_passengers = shared_mem_attach_int(shm_id);

    for (int i = 0; i < passenger_count; i++) {
        if (passenger_pids[i] > 0) {
            if (kill(passenger_pids[i], SIGKILL) == 0) {
                (*killed_passengers)++;
                printf("%d", *killed_passengers);
                printf("\033[;35m[KIEROWNIK PID=%d] Pasażer PID=%d dojechał do celu.\033[0m\n", 
                        getpid(), passenger_pids[i]);
            } else {
                perror("kill");
            }
        }
    }
    passenger_count = 0;
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGUSR1, sigusr1_handler_kierownik);  // Rejestracja sygnału 1
    signal(SIGUSR2, sigusr2_handler_kierownik);  // Rejestracja sygnału 2
    signal(SIGTERM, sigterm_handler); // Rejestracja sygnału końcowego 


    pid_t train_ID = getpid();  // PID bieżącego pociągu
    printf("\033[1;32m[KIEROWNIK PID=%d] Start pociągu.\033[0m\n", train_ID);

    int arriving_train_msq = get_message_queue(".", 2);  // Kolejka komunikatów dla przyjeżdżających pociągów
    int confirmation_msq = get_message_queue(".", 3);    // Kolejka komunikatów dla potwierdzeń

    struct message train_msg;
    train_msg.mtype = 1;  // Typ komunikatu oznaczający chęć wjazdu na stację
    train_msg.ktype = train_ID;  // PID pociągu
    
    send_message(arriving_train_msq, &train_msg);
    while (1) {
        // Powiadomienie zawiadowcy o chęci wjazdu na stację
        printf("\033[1;32m[KIEROWNIK PID=%d] Czekam na pozwolenie od zawiadowcy...\033[0m\n", train_ID);

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
            printf("\033[1;32m[KIEROWNIK PID=%d] Nie otrzymano potwierdzenia od zawiadowcy w czasie %d sekund. Próba ponownie...\033[0m\n", train_ID, T);
            continue;
        }

        printf("\033[1;32m[KIEROWNIK PID=%d] **WJEŻDŻAM NA STACJĘ I OTWIERAM DRZWI**\033[0m\n", train_ID);

        int pass_count = 0;  // Liczba pasażerów 
        int bike_count = 0;  // Liczba rowerów 
        time_t start_time = time(NULL);
    
        while ((time(NULL) - start_time < T || boarding_in_progress)){
            // Sprawdzamy, czy nie wymuszono odjazdu lub czy nie minął czas
            if (force_departure || (time(NULL) - start_time >= T)) {
                printf("\033[1;32m[KIEROWNIK PID=%d] Czas T minął lub wymuszony odjazd! Oczekuję na zakończenie wsiadania.\033[0m\n", train_ID);
                // Czekamy jeszcze chwilę, żeby ostatni pasażer mógł dokończyć wsiadanie
                sleep(1);
                // Jeśli boarding_in_progress wciąż trwa, robimy kolejną pętlę
                if (!boarding_in_progress) {
                    break;  // kończymy pętlę wsiadania
                }
            }

        if (pass_count < P) {
        struct message msg_no_bike;
            // receive_message_no_wait jest naszą funkcją do msgrcv(..., IPC_NOWAIT)
            if (receive_message_no_wait(get_message_queue(".", 0), 1, &msg_no_bike) == 1) {
                boarding_in_progress = 1;
                printf("\033[0;32m[KIEROWNIK=%d] Pasażer PID=%ld zaczyna wsiadać (bez roweru).\033[0m\n",train_ID, msg_no_bike.ktype);
                sleep(2);
                printf("\033[0;32m[KIEROWNIK=%d] Pasażer PID=%ld wsiadł (bez roweru).\033[0m\n",train_ID, msg_no_bike.ktype);     
                passenger_pids[passenger_count++] = msg_no_bike.ktype;
                pass_count++;
                boarding_in_progress = 0;
        }
    }

        if (pass_count < P && bike_count < R) {
        struct message msg_bike;
            if (receive_message_no_wait(get_message_queue(".", 1), 1, &msg_bike) == 1) {
                boarding_in_progress = 1;
                printf("\033[0;32m[KIEROWNIK=%d] Pasażer PID=%ld zaczyna wsiadać (z rowerem).\033[0m\n",train_ID, msg_bike.ktype);
                sleep(2);
                printf("\033[0;32m[KIEROWNIK=%d] Pasażer PID=%ld wsiadł (z rowerem).\033[0m\n",train_ID, msg_bike.ktype);
                passenger_pids[passenger_count++] = msg_bike.ktype;
                pass_count++;
                bike_count++;
                boarding_in_progress = 0;
        }
    }

    //usleep(20000);
}
        printf("\033[1;32m[KIEROWNIK PID=%d] **ZAMYKAM DRZWI I ODJEŻDŻAM** (Liczba pasażerów: %d, rowerów: %d)\033[0m\n", train_ID, pass_count, bike_count);

        // Powiadomienie zawiadowcy o odjeździe
        train_msg.mtype = 2;  // Typ komunikatu oznaczający odjazd
        send_message(arriving_train_msq, &train_msg);
        
        sleep(TI);  // Symulacja czasu jazdy pociągu
        printf("\033[1;32m[KIEROWNIK PID=%d] **DOJECHAŁEM – WRACAM NA STACJĘ**\033[0m\n", train_ID);

        // Oczyszczanie procesów pasażerów
        cleanup_passengers();

        sleep(TI);  // Symulacja czasu powrotu pociągu

        force_departure = 0;  // Resetowanie flagi wymuszonego odjazdu
        block_passengers = 0; // Resetowanie flagi blokującej wsiadanie pasażerów

        // Ponownie zgłoszenie chęci wjazdu
        train_msg.mtype = 1;  // Typ komunikatu oznaczający chęć wjazdu
        send_message(arriving_train_msq, &train_msg);
    }
    
    return 0;
}