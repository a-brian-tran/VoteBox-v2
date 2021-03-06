/*
 * Usage:
 *  scan [timeout in seconds]
 * 
 * Examples:
 *  Scan code, no timeout
 *      sudo ./scan
 *  Scan code, 5 second timeout
 *      sudo ./scan 5
 * 
 * Description:
 *  Scans a code using the barcode scanner on pin 25. Continues
 *  until code is successfully read or an error occurs. Valid code
 *  is printed to standard output. All printable non-whitespace characters
 *  of ASCII are supported.
 * 
 * Notes:
 *  To compile, include argument -lwiringPi, e.g.
 *      gcc scan.c -o scan -lwiringPi
 *
 *  Due to requirements of wiringPi, this program must be run as root.
 *
 * Author:
 *  Jerry Lue
 */
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <wiringPi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/input.h>
#include <limits.h>

#define MAXCODE 64 // scan buffer size
#define SCANPIN 25 // gpio pin controlling scanner on/off

// key event device of scanner
char *device = "/dev/input/by-id/usb-WIT_Electron_Company_WIT_122-UFS_V2.03-event-kbd";

/* 
 * Map linux keycodes to ASCII characters
 * 
 * Params:
 *  code    Linux keycode (from input.h)
 *  shift   Case flag (0: lowercase, 1: uppercase)
 *
 * Returns:
 *  ASCII character corresponding to keycode and shiftkey state
 */
char keymap(int code, int shift) {
    if (shift) {
        switch (code) {
            case KEY_1: return '!';
            case KEY_2: return '@';
            case KEY_3: return '#';
            case KEY_4: return '$';
            case KEY_5: return '%';
            case KEY_6: return '^';
            case KEY_7: return '&';
            case KEY_8: return '*';
            case KEY_9: return '(';
            case KEY_0: return ')';
            case KEY_MINUS: return '_';
            case KEY_EQUAL: return '+';
            case KEY_Q: return 'Q';
            case KEY_W: return 'W';
            case KEY_E: return 'E';
            case KEY_R: return 'R';
            case KEY_T: return 'T';
            case KEY_Y: return 'Y';
            case KEY_U: return 'U';
            case KEY_I: return 'I';
            case KEY_O: return 'O';
            case KEY_P: return 'P';
            case KEY_LEFTBRACE: return '{';
            case KEY_RIGHTBRACE: return '}';
            case KEY_A: return 'A';
            case KEY_S: return 'S';
            case KEY_D: return 'D';
            case KEY_F: return 'F';
            case KEY_G: return 'G';
            case KEY_H: return 'H';
            case KEY_J: return 'J';
            case KEY_K: return 'K';
            case KEY_L: return 'L';
            case KEY_SEMICOLON: return ':';
            case KEY_APOSTROPHE: return '\"';
            case KEY_GRAVE: return '~';
            case KEY_BACKSLASH: return '|';
            case KEY_Z: return 'Z';
            case KEY_X: return 'X';
            case KEY_C: return 'C';
            case KEY_V: return 'V';
            case KEY_B: return 'B';
            case KEY_N: return 'N';
            case KEY_M: return 'M';
            case KEY_COMMA: return '<';
            case KEY_DOT: return '>';
            case KEY_SLASH: return '\?';
            case KEY_SPACE: return ' ';
            default: return 0;
        }
    } else {
        switch (code) {
            case KEY_1: return '1';
            case KEY_2: return '2';
            case KEY_3: return '3';
            case KEY_4: return '4';
            case KEY_5: return '5';
            case KEY_6: return '6';
            case KEY_7: return '7';
            case KEY_8: return '8';
            case KEY_9: return '9';
            case KEY_0: return '0';
            case KEY_MINUS: return '-';
            case KEY_EQUAL: return '=';
            case KEY_Q: return 'q';
            case KEY_W: return 'w';
            case KEY_E: return 'e';
            case KEY_R: return 'r';
            case KEY_T: return 't';
            case KEY_Y: return 'y';
            case KEY_U: return 'u';
            case KEY_I: return 'i';
            case KEY_O: return 'o';
            case KEY_P: return 'p';
            case KEY_LEFTBRACE: return '[';
            case KEY_RIGHTBRACE: return ']';
            case KEY_A: return 'a';
            case KEY_S: return 's';
            case KEY_D: return 'd';
            case KEY_F: return 'f';
            case KEY_G: return 'g';
            case KEY_H: return 'h';
            case KEY_J: return 'j';
            case KEY_K: return 'k';
            case KEY_L: return 'l';
            case KEY_SEMICOLON: return ';';
            case KEY_APOSTROPHE: return '\'';
            case KEY_GRAVE: return '`';
            case KEY_BACKSLASH: return '\\';
            case KEY_Z: return 'z';
            case KEY_X: return 'x';
            case KEY_C: return 'c';
            case KEY_V: return 'v';
            case KEY_B: return 'b';
            case KEY_N: return 'n';
            case KEY_M: return 'm';
            case KEY_COMMA: return ',';
            case KEY_DOT: return '.';
            case KEY_SLASH: return '/';
            case KEY_SPACE: return ' ';
            default: return 0;
        }
    }
}

/*
 * Scan barcode.
 *
 * Params:
 *  tries   Number of tries (each try = 1 second) to attempt scan
 *
 * Returns:
 *  0 on success, -1 on error
 */
int scan(const int tries) { 
    char code[MAXCODE];
    int scanfd = open(device, O_RDONLY); 
    int err = 0, readstatus = 0, codecounter = 0, trycount = 0, shift = 0;
    FILE *scanst = fdopen(scanfd, "r");
    struct input_event keyevent;
    struct pollfd mypoll = { scanfd, POLLIN|POLLPRI };

    ioctl(scanfd, EVIOCGRAB, (void *) 1); // get exclusive access to scanner
    wiringPiSetupGpio(); // BCM pin numbering
    pinMode(SCANPIN, OUTPUT);
    // Loop until code read or error occurs
    while (trycount < tries && !err) {
        // Turn on scanner
        digitalWrite(SCANPIN, HIGH);
        // Wait for scan for 800mss before restarting scanner
        if(err = poll(&mypoll, 1, 800)) {
            while ((readstatus = read(scanfd, &keyevent, sizeof(keyevent))) >= 0) {
                #ifdef DEBUG
                printf("Read returned: %d; ", readstatus);
                printf("Event type: %u; ", keyevent.type);
                printf("Event code: %u; ", keyevent.code);
                printf("Event value: %u\n", keyevent.value);
                #endif

                if (keyevent.type == EV_KEY) { // Keyboard event
                    // Shift key up
                    if (keyevent.value == 0 &&
                            (keyevent.code == KEY_LEFTSHIFT ||
                             keyevent.code == KEY_RIGHTSHIFT)) {
                        shift = 0;
                    // Key press
                    } else if (keyevent.value == 1) {
                        // Scan complete or buffer filled, stop scanning
                        if (codecounter == MAXCODE - 1 || keyevent.code == KEY_ENTER) {
                            code[codecounter] = '\0';
                            break;
                        }
                        // Shift key down
                        if (keyevent.code == KEY_LEFTSHIFT || 
                                keyevent.code == KEY_RIGHTSHIFT) {
                            shift = 1;
                        // Convert a recognized keycode to ascii char
                        } else if (keymap(keyevent.code, shift) != 0) {
                            code[codecounter] = keymap(keyevent.code, shift);
                            codecounter++;
                        }
                    }
                }
            }
            if (readstatus < sizeof(keyevent)) {
                fprintf(stderr, "Error occurred scanning: %s", strerror(errno));
                digitalWrite(SCANPIN, LOW);
                return -1;
            }
            printf("%s\n", code);
        } else if (err < 0) {
            fprintf(stderr, "Error occurred polling: %s", strerror(errno));
            digitalWrite(SCANPIN, LOW);
            return -1;
        }
        //else {
        //    printf("No code scanned\n");
        //}

        // Restart scanner. Wait 200ms to allow it to reset
        digitalWrite(SCANPIN, LOW);
        usleep(200000);
        trycount ++;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    switch (argc) {
        case 1:
            return scan(INT_MAX);
        case 2:
            return scan(atoi(argv[1]));
        default:
            return scan(INT_MAX);
    }
}
