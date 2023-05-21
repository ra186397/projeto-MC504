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

void imprimir_salas(int id_aluno, int sala, int entrada) {

    if (diretor == 3) {
        printf("           D\n");
    } else if (diretor == 4) {
        printf("                                 D\n");
    }

    printf("        Sala 1                Sala 2\n");
    printf("+---------------------+---------------------+\n");

    // Desenhar contadores de alunos da Sala 1

    char festa1;
    char festa2;

    if (salas[0] >= N_FESTA && diretor != 1) {
        festa1 = 'P';
    } else {
        festa1 = ' ';
    }

    if (salas[1] >= N_FESTA && diretor != 2) {
        festa2 = 'P';
    } else {
        festa2 = ' ';
    }

    printf("|      Alunos: %2d %c   ", salas[0], festa1);

    // Desenhar contadores de alunos da Sala 2
    printf("|      Alunos: %2d %c   ", salas[1], festa2);
    printf("|\n");

    // Desenhar linha inferior com símbolo do diretor
    printf("|         ");
    if (diretor == 1) {
        printf(" D          |                     |\n");
    } else if (diretor == 2) {
        printf("            |          D          |\n");
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
    if (aleatorio % 2 == 0) { // decide que sala o estudante vai checar aleatoriamente. Sala1 é 0, Sala2 é 1.
        numSala = 0;
    }
    else {
        numSala = 1;
    }

    if (diretor == 1 + numSala) { // Diretor na sala escolhida

        sem_post(&sem_mutex);
        sem_wait(&sem_diretorPresente);
        sem_post(&sem_diretorPresente);
        sem_wait(&sem_mutex);
    }

    salas[numSala] += 1;
    
    imprimir_salas(ra, numSala, 1);

    if (salas[numSala] == N_FESTA && diretor == 3 + numSala) { // Diretor entra para parar a festa.
        sem_post(&sem_emboscando);
    }
    else {
        sem_post(&sem_mutex);
    }
    
    sem_wait(&sem_mutex);

    salas[numSala] -= 1;

    imprimir_salas(ra, numSala, 0);
    
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

        if (salas[numSala] > 0 && salas[numSala] < N_FESTA)
        {
            diretor = 3 + numSala;
            sem_post(&sem_mutex);
            sem_wait(&sem_emboscando); // Passa a vez para os estudantes.
        }

        if (salas[numSala] >= N_FESTA) {
            diretor = 1 + numSala;
            sem_wait(&sem_diretorPresente); // Bloqueia novos estudantes de entrar.
            sem_post(&sem_mutex);           // Passa a vez para os estudantes.

            if (diretor == 1) {
                sem_wait(&sem_esvaziar1);       // Espera a sala esvaziar.
            }
            else {
                sem_wait(&sem_esvaziar2);
            }

            sem_post(&sem_diretorPresente); // Permite estudantes entrarem.
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
        pthread_join(thr_estudantes[i], NULL);
    }

    return 0;
}