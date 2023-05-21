#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <windows.h>

#define N_ESTUDANTES 100
#define N_FESTA N_ESTUDANTES / 10

int salas[2];
typedef enum {Parado = 0, Sala1 = 1, Sala2 = 2, Esperando1 = 3, Esperando2 = 4}Diretor;
Diretor diretor = 0;
sem_t sem_mutex;
sem_t sem_diretorPresente;
sem_t sem_esvaziar1;
sem_t sem_esvaziar2;
sem_t sem_emboscando;

void imprimir_salas(int id_aluno, int posicao_diretor, int salas[], int sala, int entrada) {

    if (posicao_diretor == 3) {
        printf("           D                        \n");
    } else if (posicao_diretor == 4) {
        printf("                         D           \n");
    }

    printf("        Sala 1                Sala 2\n");
    printf("+---------------------+---------------------+\n");

    // Desenhar contadores de alunos da Sala 1
    printf("|       Alunos: %2d %c   ", salas[0], (salas[0] >= N_FESTA ? "P" : " "));

    // Desenhar contadores de alunos da Sala 2
    printf("|       Alunos: %2d %c   ", salas[1], (salas[1] >= N_FESTA ? "P" : " "));
    printf("|\n");

    // Desenhar linha inferior com símbolo do diretor
    printf("|         ");
    if (posicao_diretor == 1) {
        printf(" D          |                     |\n");
    } else if (posicao_diretor == 2) {
        printf("                |          D          |\n");
    } else {
        printf("            |                     |\n");
    }

    // Desenhar portas de entrada e saída da Sala 1
    printf("|    ");
    if (sala == 0 && entrada == 1) {
        printf(" E %2d ", id_aluno);
    } else {
        printf("      ");
    }
    printf("  ");
    if (sala == 0 && entrada == 0) {
        printf(" S %2d ", id_aluno);
    } else {
        printf("      ");
    }
    printf("   |");

    // Desenhar portas de entrada e saída da Sala 2
    printf("    ");
    if (sala == 1 && entrada == 1) {
        printf(" E %2d ", id_aluno);
    } else {
        printf("      ");
    }
    printf("  ");
    if (sala == 1 && entrada == 0) {
        printf(" S %2d ", id_aluno);
    } else {
        printf("      ");
    }
    printf("   |\n");

    printf("+---------------------+---------------------+\n");

    return 0;
}

void* f_estudante(void* v) {

    int ra = *(int*) v;
    int numSala;
    Sleep(2.75 + ra/(N_ESTUDANTES * 2));
    srand(ra);
    sem_wait(&sem_mutex);
    int aleatorio = rand();
    printf("%d\n", aleatorio);
    if (aleatorio % 2 == 0) { // decide que sala o estudante vai checar aleatoriamente. Sala1 é 0, Sala2 é 1.
        numSala = 0;
    }
    else {
        numSala = 1;
    }

    if (diretor == 1 + numSala) { // Diretor na sala escolhida

        printf("O estudante %d tentou entrar na sala %d mas foi barrado pelo diretor.\n", ra, numSala + 1);
        sem_post(&sem_mutex);
        sem_wait(&sem_diretorPresente);
        sem_post(&sem_diretorPresente);
        sem_wait(&sem_mutex);
    }

    salas[numSala] += 1;
    printf("O estudante %d entrou na sala %d. O total aumentou para %d.\n", ra, numSala + 1, salas[numSala]);
    
    imprimir_salas(ra, diretor, salas, numSala, 0);

    if (salas[numSala] == N_FESTA && diretor == 3 + numSala) { // Diretor entra para parar a festa.
        sem_post(&sem_emboscando);
    }
    else {
        sem_post(&sem_mutex);
    }

    if (salas[numSala] == N_FESTA) {
        printf("Tem bastante gente na sala %d! Hora da festa! Oba oba!\n", numSala + 1);
    }
    
    sem_wait(&sem_mutex);

    salas[numSala] -= 1;
    printf("O estudante %d saiu da sala %d. O total caiu para %d.\n", ra, numSala + 1, salas[numSala]);

    if (salas[numSala] == 0 && diretor == 3 + numSala) { // Diretor entra para vasculhar.
        sem_post(&sem_emboscando);
    }
    else if (salas[numSala] == 0 && diretor == 1) {
        sem_post(&sem_esvaziar1);
    }
    else if (salas[numSala] == 0 && diretor == 2) {
        sem_post(&sem_esvaziar2);
    }
    else {
        sem_post(&sem_mutex);
    }

    imprimir_salas(ra, diretor, salas, numSala, 0);
}

void* f_diretor(void *v) {
    while(1) {
        sem_wait(&sem_mutex);
        int numSala;
        if (rand() % 2 == 0) { // decide que sala o diretor vai checar aleatoriamente. Sala1 é 0, Sala2 é 1.
            numSala = 0;
        }
        else {
            numSala = 1;
        }

        printf("O diretor decidiu emboscar a sala %d.\n", numSala + 1);

        if (salas[numSala] > 0 && salas[numSala] < N_FESTA)
        {
            diretor = 3 + numSala;
            sem_post(&sem_mutex);
            sem_wait(&sem_emboscando); // Passa a vez para os estudantes.
        }

        if (salas[numSala] >= N_FESTA) {
            diretor = 1 + numSala;
            printf("'Nada de festa na sala %d. Saiam todos agora!'\n", numSala + 1);
            sem_wait(&sem_diretorPresente); // Bloqueia novos estudantes de entrar.
            sem_post(&sem_mutex);           // Passa a vez para os estudantes.
            sem_post(&sem_diretorPresente); // Permite estudantes entrarem.

            if (diretor == 1) {
                sem_wait(&sem_esvaziar1);       // Espera a sala esvaziar.
            }
            else {
                sem_wait(&sem_esvaziar2);
            }
        }
        else {
            printf("'O que que tem de bom aqui na sala %d, hein?\n", numSala + 1);
        }

        diretor = 0;
        sem_post(&sem_mutex);
    }
    return NULL;

}


int main() {
    pthread_t thr_estudantes[N_ESTUDANTES], thr_diretor;
    int i, ra[N_ESTUDANTES];

    sem_init(&sem_mutex,0,1);
    sem_init(&sem_diretorPresente,0,1);
    sem_init(&sem_esvaziar1,0,0);
    sem_init(&sem_esvaziar2,0,0);
    sem_init(&sem_emboscando,0,0);


    pthread_create(&thr_diretor, NULL, f_diretor, NULL);

    for (i = 0; i < N_ESTUDANTES; i++)
    {
        ra[i] = i;
        pthread_create(&thr_estudantes[i], NULL, f_estudante, (void*) &ra[i]);
    }

    for (i = 0; i < N_ESTUDANTES; i++)
    {
        pthread_join(thr_estudantes[i], NULL); // não fazer bosta antes de outra thread terminar
    }

    return 0;
}