#include <stdio.h>
#include <stdlib.h>
#define MAX_SCORES 100

typedef struct {
    char name[20];
    int score;
} ScoreEntry;

char username [20];

ScoreEntry scores[MAX_SCORES];
int scoreCount = 0;

void load_scores(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        scoreCount = 0;  // no file yet
        return;
    }

    scoreCount = 0;
    while (scoreCount < MAX_SCORES &&
           fscanf(fp, "%19s %d", scores[scoreCount].name,
                                 &scores[scoreCount].score) == 2)
    {
        scoreCount++;
    }

    fclose(fp);
}

void add_score(const char *filename, const char *playerName, int score) {
    load_scores(filename);

    if (scoreCount < MAX_SCORES) {
        strcpy(scores[scoreCount].name, playerName);
        scores[scoreCount].score = score;
        scoreCount++;
    }
    else {
        sort_scores();

        if (score < scores[scoreCount - 1].score) {
            strcpy(scores[scoreCount - 1].name, playerName);
            scores[scoreCount - 1].score = score;
        }
    }

    sort_scores();
    save_scores(filename);
}

int compare_scores(const void *a, const void *b) {
    const ScoreEntry *A = (const ScoreEntry *)a;
    const ScoreEntry *B = (const ScoreEntry *)b;

    return A->score - B->score;
}

void sort_scores() {
    qsort(scores, scoreCount, sizeof(ScoreEntry), compare_scores);
}

void save_scores(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;

    for (int i = 0; i < scoreCount; i++) {
        fprintf(fp, "%s %d\n", scores[i].name, scores[i].score);
    }

    fclose(fp);
}

void print_scores() {
    for (int i = 0; i < scoreCount; i++) {
        printf("%d. %s - %d\n", i+1, scores[i].name, scores[i].score);
    }
}

void get_player_name(char *username) {
    printf("Enter name: ");
    if (fgets(username, 20, stdin) == NULL){
        printf("Error: failed to read name.\n");
        username[0] = '\0';
        return;
    }

    username[strcspn(username, "\n")] = 0;
}