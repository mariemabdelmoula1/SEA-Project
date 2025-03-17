#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>  // Pour utiliser clock()

#define MAX_STUDENTS 100
#define NUM_THREADS 4

// Structure représentant un étudiant
typedef struct {
    char name[50];
    char absence;
} Student;

// Structure pour passer plusieurs arguments aux threads
typedef struct {
    Student* students;  // Tableau des étudiants
    int numStudents;    // Nombre d'étudiants
    int start;          // Indice de départ pour le thread
    int end;            // Indice de fin pour le thread
} ThreadArgs;

// Queue partagée
Student absentQueue[MAX_STUDENTS];
int queueFront = 0, queueRear = 0;
pthread_mutex_t queueMutex;
sem_t emptySlots, fullSlots;
int producerCount = 0;  // Compte les producteurs ayant terminé

// Fonction producteur pour ajouter un étudiant à la queue
void produce(Student student) {
    absentQueue[queueRear] = student;
    queueRear = (queueRear + 1) % MAX_STUDENTS;
}

// Fonction consommateur pour afficher un étudiant de la queue
void consume() {
    if (queueFront != queueRear) {
        Student student = absentQueue[queueFront];
        queueFront = (queueFront + 1) % MAX_STUDENTS;
        printf("Absent: %s\n", student.name);
    }
}

// Fonction exécutée par les threads producteurs
void* processAbsentStudents(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    Student* students = args->students;
    int start = args->start;
    int end = args->end;

    for (int i = start; i < end; i++) {
        if (students[i].absence == 'A') {
            sem_wait(&emptySlots);  // Attendre de l'espace dans la queue
            pthread_mutex_lock(&queueMutex);  // Protection de la queue
            produce(students[i]);
            pthread_mutex_unlock(&queueMutex);
            sem_post(&fullSlots);  // Signaler que la queue contient un élément
        }
    }

    // Incrémenter le compteur des producteurs qui ont terminé
    __sync_fetch_and_add(&producerCount, 1);
    return NULL;
}

// Fonction exécutée par le thread consommateur
void* displayAbsentStudents(void* arg) {
    while (1) {
        sem_wait(&fullSlots);  // Attendre qu'un élément soit ajouté
        pthread_mutex_lock(&queueMutex);  // Protection de la queue
        consume();
        pthread_mutex_unlock(&queueMutex);
        sem_post(&emptySlots);  // Signaler qu'il y a de la place dans la queue

        // Arrêter la boucle lorsque tous les producteurs ont terminé
        if (producerCount == NUM_THREADS && queueFront == queueRear) {
            break;
        }
    }
    return NULL;
}

int main() {
    clock_t start_time, end_time;  // Variables pour mesurer le temps
    double time_taken;  // Variable pour stocker le temps d'exécution

    start_time = clock();  // Démarrer le chronomètre

    FILE *file = fopen("students.csv", "r");
    if (file == NULL) {
        printf("Erreur d'ouverture du fichier.\n");
        return 1;
    }

    Student students[MAX_STUDENTS];
    int numStudents = 0;
    char line[256];

    fgets(line, sizeof(line), file);  // Lire et ignorer l'en-tête
    while (fgets(line, sizeof(line), file) && numStudents < MAX_STUDENTS) {
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%[^,],%c", students[numStudents].name, &students[numStudents].absence) == 2) {
            numStudents++;
        }
    }
    fclose(file);

    // Vérification de la lecture des données
    if (numStudents == 0) {
        printf("⚠ Aucune donnée chargée. Vérifiez le fichier CSV.\n");
        return 1;
    }

    // Initialiser le mutex et les sémaphores
    pthread_mutex_init(&queueMutex, NULL);
    sem_init(&emptySlots, 0, MAX_STUDENTS);  // Nombre maximal de places dans la queue
    sem_init(&fullSlots, 0, 0);  // Commence avec une queue vide

    pthread_t threads[NUM_THREADS];
    pthread_t consumerThread;

    // Créer les threads producteurs
    for (int i = 0; i < NUM_THREADS; i++) {
        // Créer un objet ThreadArgs pour chaque thread
        ThreadArgs* threadArgs = malloc(sizeof(ThreadArgs));
        threadArgs->students = students;
        threadArgs->numStudents = numStudents;
        threadArgs->start = i * (numStudents / NUM_THREADS);  // Indice de départ
        threadArgs->end = (i + 1) * (numStudents / NUM_THREADS);  // Indice de fin
        
        // Gérer les cas où les étudiants ne sont pas divisibles exactement
        if (i == NUM_THREADS - 1) {
            threadArgs->end = numStudents;  // Dernier thread prend le reste des étudiants
        }

        pthread_create(&threads[i], NULL, processAbsentStudents, (void*)threadArgs);
    }

    // Créer le thread consommateur
    pthread_create(&consumerThread, NULL, displayAbsentStudents, NULL);

    // Attendre la fin des threads producteurs
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Signaler au consommateur qu'il y a des éléments à consommer
    sem_post(&fullSlots);

    // Attendre la fin du thread consommateur
    pthread_join(consumerThread, NULL);

    // Libérer les ressources
    sem_destroy(&emptySlots);
    sem_destroy(&fullSlots);
    pthread_mutex_destroy(&queueMutex);

    end_time = clock();  // Arrêter le chronomètre
    time_taken = ((double)end_time - start_time) / CLOCKS_PER_SEC;  // Calculer le temps écoulé en secondes

    printf("Temps d'exécution: %.6f secondes\n", time_taken);  // Afficher le temps d'exécution

    return 0;
}
