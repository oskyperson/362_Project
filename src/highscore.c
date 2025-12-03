#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdcard.h"
#include "ff.h"


void append_to_file(const char *filename, int score)
{
    FIL fil;
    FRESULT fr;

    // Open file in append mode
    fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_EXISTING | FA_OPEN_APPEND);
    if (fr) {
        print_error(fr, filename);
        return;
    }

    char line[20];
    snprintf(line, sizeof(line), "%d\n", score);

    // Write to file
    UINT wlen;
    fr = f_write(&fil, line, strlen(line), &wlen);
    if (fr || wlen != strlen(line)) {
        print_error(fr, filename);
    }

    f_close(&fil);
}


// void get_player_name(char *username) {
//     printf("Enter name: ");
//     if (fgets(username, 20, stdin) == NULL){
//         printf("Error: failed to read name.\n");
//         username[0] = '\0';
//         return;
//     }

//     username[strcspn(username, "\n")] = 0;
// }