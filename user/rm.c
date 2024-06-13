#include <lib.h>
int main(int argc, char **argv) {
    char *path;
    int r = 0;
    int f = 0;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3) {
        path = argv[2];
        if (strcmp(argv[1], "-r") == 0) {
            r = 1;
        } else {
            
        }
    }
    return 0;
}