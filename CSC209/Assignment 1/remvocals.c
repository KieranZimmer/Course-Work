#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <source wav> <dest wav>", argv[0]);
        return -1;
    }
    FILE *source, *output;
    source = fopen(argv[1], "rb");
    if (!source) {
        perror("File doesnt exist");
        return -1;
    }
    output = fopen(argv[2], "wb");
    if (!output) {
        perror("Couldn't create file");
        return -1;
    }
    int m, n;
    short x, left, right, i;
    for (i = 0;i < 22;i++) {
        m = fread(&x, sizeof(x), 1, source);
        if (!m) {
            perror("read error, header");
            return -2;
        }
        n = fwrite(&x, sizeof(x), 1, output);
        if (!n) {
            perror("write error, header");
            return -2;
        }
    }
    m = fread(&left, sizeof(left), 1, source) && fread(&right, sizeof(right), 1, source);
    while (m) {
        x = (left - right) / 2;
        n = fwrite(&x, sizeof(x), 1, output) && fwrite(&x, sizeof(x), 1, output);
        if (n != 1) {
            perror("write error, samples");
            return -3;
        }
        m = fread(&left, sizeof(left), 1, source) && fread(&right, sizeof(right), 1, source);
    }
    fclose(source);
    fclose(output);
    return 0;
}
