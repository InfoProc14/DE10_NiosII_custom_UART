#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* message = "Hello World\n";

    alt_putstr("Opening the UART file.\n"); 
    FILE* fp = fopen("/dev/uart", "r+");

    if(fp) {
        alt_putstr("SUCCESS: /dev/uart opened\n");
    }

    while(1){
        fwrite(message, strlen(message), 1, fp);
    }

    alt_putstr("Closing the UART file.\n"); 
    fclose (fp);

    return 0;
}