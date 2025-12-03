#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdcard.h"
#include "ff.h"


void append_to_file(const char *filename, char *name)
{
    FIL fil;
    char line[100];
    FRESULT fr;

    // Open file in append mode
    fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_EXISTING | FA_OPEN_APPEND);
    if (fr) {
        print_error(fr, filename);
        return;
    }

    // Write to file
    UINT wlen;
    fr = f_write(&fil, name, strlen(name), &wlen);
    if (fr || wlen != strlen(name)) {
        print_error(fr, filename);
    }

    f_close(&fil);
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