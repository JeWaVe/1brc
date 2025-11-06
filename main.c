#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEYS (1 << 20)
#define MASK (MAX_KEYS - 1)

typedef struct Station {
    char* name;
    float min;
    float max;
    float total;
    int count;
} Station_t;

void print_station(Station_t* s) {
    printf("%s : \n", s->name);
    printf("\t%d %f %f %f\n", s->count, s->min, s->max, s->total/(float)s->count);
}

// vibe coded this function
int get_proc_count() {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    int phys_ids[512];
    int core_ids[512];
    int n = 0;

    char line[256];
    int phys=-1, core=-1;

    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "physical id\t: %d", &phys) == 1) continue;
        if (sscanf(line, "core id\t\t: %d", &core)  == 1) continue;

        if (line[0] == '\n') {
            if (phys >= 0 && core >= 0) {
                int seen = 0;
                for (int i=0; i<n; i++) {
                    if (phys_ids[i] == phys && core_ids[i] == core) {
                        seen = 1;
                        break;
                    }
                }
                if (!seen) {
                    phys_ids[n] = phys;
                    core_ids[n] = core;
                    n++;
                }
            }
            phys = core = -1;
        }
    }
    fclose(f);

    return n;
}

float mystrtof(const char* buff, int length) {
    int sign = buff[0] == '-';
    int start = sign ? 1 : 0;
    float mult = 0.1;
    float res = 0;
    for(int i = length - 2; i >= start; --i) {
        char c = buff[i];
        if(c == '.')
            continue;
        res += mult*(c - '0');
        mult *= 10;
    }
    return sign ? -res : res;
}

void readsegment(const char* buff, size_t from, size_t to, size_t len, Station_t* stations) {
    while(from > -1 && buff[from--] != '\n') {} // rewind from to previous linefeed
    if(from < 5) {
        from = 0;
    }
    while(to < len && buff[to++] != '\n') {} // push to to last linefeed or EOF
    to--;

    unsigned char station_name[32] = {0};
    unsigned char temp[8] = {0};

    while(from < to) {
        int w_i = 0;
        int station_length = 0;
        unsigned hash = 146527;
        while(buff[from] != ';') {
            unsigned char c = (unsigned char) buff[from++];
            station_name[w_i++] = c;
            hash = (hash * 33) ^ c;
        }

        hash &= MASK;

        from++;
        station_length = w_i+1;

        w_i = 0;

        // TODO: replace that (32%) by something smarter
        // get rid of stack variable
        // read data once
        while(buff[from] != '\n') {
            temp[w_i++] = buff[from++];
        }
        int tmp_length = w_i+1;

        float temperature = mystrtof(temp, tmp_length);
        // TODO: mutex
        Station_t* station = &stations[hash];
        if(station->name == NULL) {
            station->name = malloc(station_length);
            strcpy(station->name, station_name);
            station->min = temperature;
            station->max = temperature;
            station->total = temperature;
            station->count = 1;
        } else {
            // if(strcmp(station_name, station->name) != 0) {
            //     printf("conflict : %s -- %s have same hash\n", station_name, station->name);
            //     abort();
            // }
            if(temperature < station->min) {
                station->min = temperature;
            }
            if(temperature > station->max) {
                station->max = temperature;
            }
            station->total += temperature;
            station->count += 1;
        }
        // !TODO

        from++;
        memset(station_name, 0, sizeof(station_name));
        memset(temp, 0, sizeof(temp));
    }
}

int main(int argc, char *argv[]) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int processor_count = get_proc_count();
    printf("running with %d processors\n", processor_count);
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t len = st.st_size;
    char *buf = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    size_t step = len / processor_count;

    Station_t* stations = (Station_t*) malloc(MAX_KEYS * sizeof(Station_t));
    for(int i = 0; i < processor_count; ++i) {
        size_t from = (size_t) (i * step);
        size_t to = from + step;
        printf("READ SEGMENT %d\n", i);
        readsegment(buf, from, to, len, stations);
    }

    munmap(buf, len);
    close(fd);
    free(stations);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("elapsed: %.6f s\n", elapsed);
    return 0;
}
