#include <lib.h>
int main(int argc, char **argv) {
    if (argc != 2) {
        debugf("touch: use touch <filename>\n");
        return 0;
    }
    int r = open(argv[1], O_RDONLY);
    if (r >= 0) {
        printf("touch: cannot touch \'%s\': File exists\n", argv[1]);
        close(r);
        return 0;
    } else {
        if ((r = create(argv[1], FTYPE_REG)) != 0) {
            printf("touch: cannot touch \'%s\': No such file or directory\n", argv[1]);
            return 1;
        }
        return 0;
    }
}