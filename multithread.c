#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define MAX_STUDENTS 100
#define MAX_NAME_LENGTH 50
#define NUM_THREADS 4  // Nombre de threads

// Structure représentant un étudiant
typedef struct {
    char name[MAX_NAME_LENGTH];
    char absence;
} Student;

typedef struct {
    Student* students;
    int start;
    int end;
} ThreadData;

pthread_mutex_t mutex; // Mutex pour protéger l'affichage

// Fonction exécutée par chaque thread pour afficher les étudiants absents
void* printAbsentStudents(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    for (int i = data->start; i < data->end; i++) {
        if (data->students[i].absence == 'A') {
            pthread_mutex_lock(&mutex); // Protection de l'affichage
            printf("%s\n", data->students[i].name);
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

int main() {
    FILE *file = fopen("students.csv", "r");
    if (file == NULL) {
        printf("Erreur d'ouverture du fichier.\n");
        return 1;
    }

    Student students[MAX_STUDENTS];
    int numStudents = 0;
    char line[256];

    // Lire et ignorer l'en-tête
    fgets(line, sizeof(line), file);

    // Lire le fichier et stocker les étudiants
    while (fgets(line, sizeof(line), file) && numStudents < MAX_STUDENTS) {
        // Supprimer le retour à la ligne s'il existe
        line[strcspn(line, "\r\n")] = 0;

        // Lire le nom et le statut d'absence
        if (sscanf(line, "%[^,],%c", students[numStudents].name, &students[numStudents].absence) == 2) {
            numStudents++;
        }
    }
    fclose(file);

    // Vérification si la dernière ligne est bien prise en compte
    if (numStudents == 0) {
        printf("⚠ Aucune donnée chargée. Vérifiez le fichier CSV.\n");
        return 1;
    }

    // Initialiser le mutex
    pthread_mutex_init(&mutex, NULL);

    // Mesurer le temps d'exécution
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    pthread_t threads[NUM_THREADS];
    ThreadData threadData[NUM_THREADS];

    int studentsPerThread = numStudents / NUM_THREADS;
    int extraStudents = numStudents % NUM_THREADS;

    // Créer les threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].students = students;
        // Répartition des étudiants, avec gestion des restes
        threadData[i].start = i * studentsPerThread + (i < extraStudents ? i : extraStudents);
        threadData[i].end = threadData[i].start + studentsPerThread + (i < extraStudents ? 1 : 0);
        
        pthread_create(&threads[i], NULL, printAbsentStudents, &threadData[i]);
    }

    // Attendre la fin des threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Détruire le mutex
    pthread_mutex_destroy(&mutex);

    // Fin de la mesure du temps
    gettimeofday(&end_time, NULL);
    double execution_time = (double)(end_time.tv_sec - start_time.tv_sec) + 
                            (double)(end_time.tv_usec - start_time.tv_usec) / 1000000;
    printf("Temps d'exécution total : %.6f secondes\n", execution_time);

    return 0;
}
