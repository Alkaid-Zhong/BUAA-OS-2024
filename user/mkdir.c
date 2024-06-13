#include <lib.h>
int main(int argc, char **argv) {
    char *path;
    int p = 0;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        p = 1;
        path = argv[2];
    } else {
        debugf("mkdir: use mkdir [-p] <dirname>\n");
        return 0;
    }
    int r = open(path, O_RDONLY);
    if (r >= 0) {
        printf("mkdir: cannot create directory '%s': File exists\n", path);
        close(r);
        return 0;
    } else {
        if ((r = create(argv[1], FTYPE_DIR)) != 0) {
            if (!p) {
                printf("mkdir: cannot create directory '%s': No such file or directory\n", path);
                return 1;
            }
        }
    }
    return 0;
}