#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define HEADER_SIZE 22

int main(int argc, char **argv) {
    FILE *source, *output;
    int delay = 8000, vol = 4, m, n, i, opt;
    while ((opt = getopt(argc, argv, "d:v:")) != -1) {
        if (opt == 'd') delay = atoi(optarg);
        if (opt == 'v') vol = atoi(optarg);
    }
    unsigned int * size1, * size2, totL, origL;
    short header[HEADER_SIZE], x, y, *echo;
    echo = malloc(delay * sizeof(short)); //what about when delay > samples in original
    if (!echo) {
        printf("Malloc fail");
        return -1;
    }
    source = fopen(argv[argc - 2], "rb");
    if (!source) {
        perror("File doesnt exist");
        return -1;
    }
    output = fopen(argv[argc - 1], "wb");
    if (!output) {
        perror("Couldn't create file");
        return -1;
    }

    for (i = 0;i < 22;i++) { //read header, loop not strictly necessary
        m = fread(header + i, sizeof(header[0]), 1, source);
        if (!m) {
            perror("read error, header");
            return -2;
        }
    }
    size1 = (unsigned int *)(header + 2);
    size2 = (unsigned int *)(header + 20);
    origL = *size2 / 2;
    *size1 += 2*delay;
    *size2 += 2*delay;
    totL = *size2 / 2;

    for (i = 0;i < 22;i++) { //read header
        if (i == 2) {
            n = fwrite(size1, sizeof(*size1), 1, output);
            i++;
        } else if (i == 20) {
            n = fwrite(size2, sizeof(*size2), 1, output);
            i++;
        }
        else n = fwrite(header + i, sizeof(header[0]), 1, output);
        if (!n) {
            perror("write error, header");
            return -2;
        }
    }

    int j = 0;

    for (i = 0;i < totL;i++) {
        if (i < delay && i < origL) {
            m = fread(&x, sizeof(x), 1, source);
            if (!m) {
                perror("read error 1");
                return -2;
            }
            n = fwrite(&x, sizeof(x), 1, output);
            if (!n) {
                perror("write error 1");
                return -2;
            }
            echo[i] = x;
        }
        else if (i < delay && i >= origL) {
            x = 0;
            n = fwrite(&x, sizeof(x), 1, output);
            if (!n) {
                perror("write error 2");
                return -2;
            }
        }
        else if (i >= delay && i < origL) {
            m = fread(&x, sizeof(x), 1, source);
            if (!m) {
                perror("read error 3 ");
                return -2;
            }
            y = x + echo[j] / vol;
            echo[j] = x;
            j++;
            if (j >= delay) j = 0;
            n = fwrite(&y, sizeof(y), 1, output);
            if (!n) {
                perror("write error 3");
                return -2;
            }
        }
        else if (i >= delay && i >= origL) {
            x = echo[j] / vol;
            j++;
            if (j >= delay) j = 0;
            n = fwrite(&x, sizeof(x), 1, output);
            if (!n) {
                perror("write error 4");
                return -2;
            }
        }
    }

    free(echo);
    fclose(source);
    fclose(output);
    return 0;
}
