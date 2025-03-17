#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <string.h>

#define MAX_STUDENTS 100
#define MAX_NAME_LENGTH 50

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

typedef struct {
    char name[MAX_NAME_LENGTH];
    char absence;
} Student;

void writeAbsentStudents(Student students[], int start, int end, char* shared_memory, int offset, int processID) {
    char buffer[MAX_STUDENTS * (MAX_NAME_LENGTH + 50)];
    buffer[0] = '\0'; 

    for (int i = start; i < end; i++) {
        if (students[i].absence == 'A') {
            char temp[MAX_NAME_LENGTH + 50];
            snprintf(temp, sizeof(temp), "Process %d: %s\n", processID, students[i].name);
            strcat(buffer, temp);
        }
    }

    strcpy(shared_memory + offset, buffer);  
}

void wait_semaphore(int sem_id, int sem_index) {
    struct sembuf sem_op;
    sem_op.sem_num = sem_index;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}

void signal_semaphore(int sem_id, int sem_index) {
    struct sembuf sem_op;
    sem_op.sem_num = sem_index;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}

int main() {
    FILE *file = fopen("students.csv", "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        return 1;
    }

    Student students[MAX_STUDENTS];
    int numStudents = 0;

    char line[256];
    fgets(line, sizeof(line), file);
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%c", students[numStudents].name, &students[numStudents].absence);
        numStudents++;
    }

    fclose(file);

    int numProcesses = 4; // Nombre de processus
    int studentsPerProcess = numStudents / numProcesses;
    int extraStudents = numStudents % numProcesses;

    key_t key = ftok("students.csv", 'A');
    int sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        return 1;
    }
    union semun arg;
    arg.val = 1;
    semctl(sem_id, 0, SETVAL, arg);

    // La mémoire partagée pour stocker les résultats
    int shm_id = shmget(IPC_PRIVATE, MAX_STUDENTS * (MAX_NAME_LENGTH + 50) * numProcesses, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        return 1;
    }
    char *shared_memory = (char *)shmat(shm_id, NULL, 0);
    if (shared_memory == (char *)-1) {
        perror("shmat");
        return 1;
    }

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < numProcesses; i++) {
        // Répartition des étudiants parmi les processus
        int start = i * studentsPerProcess + (i < extraStudents ? i : extraStudents);
        int end = start + studentsPerProcess + (i < extraStudents ? 1 : 0);
        
        int offset = i * MAX_STUDENTS * (MAX_NAME_LENGTH + 50);

        int pid = fork();
        if (pid == 0) {
            // Chaque processus attend le sémaphore avant de commencer
            wait_semaphore(sem_id, 0);

            writeAbsentStudents(students, start, end, shared_memory, offset, getpid());

            // Libération du sémaphore après l'écriture
            signal_semaphore(sem_id, 0);
            shmdt(shared_memory); 
            exit(0);
        } else if (pid > 0) {
            continue;
        } else {
            printf("Fork failed.\n");
            return 1;
        }
    }

    // Attente de la fin de tous les processus enfants
    for (int i = 0; i < numProcesses; i++) {
        wait(NULL);
    }

    // Affichage des résultats collectés dans la mémoire partagée
    for (int i = 0; i < numProcesses; i++) {
        int offset = i * MAX_STUDENTS * (MAX_NAME_LENGTH + 50);
        printf("%s", shared_memory + offset);
    }

    // Détachement et nettoyage de la mémoire partagée
    shmdt(shared_memory);
    shmctl(shm_id, IPC_RMID, NULL);

    gettimeofday(&end_time, NULL);
    double execution_time = (double)(end_time.tv_sec - start_time.tv_sec) + (double)(end_time.tv_usec - start_time.tv_usec) / 1000000;

    printf("Total execution time: %.6f seconds\n", execution_time);

    // Suppression du sémaphore
    semctl(sem_id, 0, IPC_RMID, arg);

    return 0;
}
