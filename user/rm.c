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
        }
        if (strcmp(argv[1], "-rf") == 0) {
            r = 1;
            f = 1;
        }
    } else {
        debugf("rm: use rm [-r|f] <filename|dirname>\n");
        return 0;
    }
    int r = open(path, O_RDONLY);
    return 0;
}