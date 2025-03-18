#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>      // Pour gettid() sous Linux
#include <sys/syscall.h> // Pour syscall(SYS_gettid)

#define MAX_STUDENTS 100
#define MAX_NAME_LENGTH 50

// Structure représentant un étudiant
typedef struct {
    char name[MAX_NAME_LENGTH];
    char absence;
} Student;

// Fonction pour afficher les étudiants absents
void printAbsentStudents(Student students[], int numStudents) {
    printf("Thread ID dans printAbsentStudents: %ld\n", syscall(SYS_gettid)); // Affiche l'ID du thread
    printf("Liste des étudiants absents :\n");
    for (int i = 0; i < numStudents; i++) {
        if (students[i].absence == 'A') {
            printf("%s\n", students[i].name);
        }
    }
}

int main() {
    printf("Thread ID dans main: %ld\n", syscall(SYS_gettid)); // Affiche l'ID du thread principal

    // Ouvrir le fichier CSV contenant les étudiants
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
        sscanf(line, "%[^,],%c", students[numStudents].name, &students[numStudents].absence);
        numStudents++;
    }
    fclose(file);

    // Mesurer le temps d'exécution
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Afficher les étudiants absents
    printAbsentStudents(students, numStudents);

    // Fin de la mesure du temps
    gettimeofday(&end_time, NULL);
    double execution_time = (double)(end_time.tv_sec - start_time.tv_sec) + 
                            (double)(end_time.tv_usec - start_time.tv_usec) / 1000000;
    printf("Temps d'exécution total : %.6f secondes\n", execution_time);

    return 0;
}
