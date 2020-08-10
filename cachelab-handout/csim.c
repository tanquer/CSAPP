#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "cachelab.h"

FILE *infile;

typedef struct {
    _Bool valid;
    int timestamp;
    long long tag;
} _cache_line;

_Bool verbose_flag = false;
int hit_total, miss_total, eviction_total;
int s, E, b, file_index, time;
long long S, set_mask, tag_mask = -1;

/* 
 * usage - Display usage info
 */
static void usage(char *cmd) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", cmd);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", cmd);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", cmd);
    exit(1);
}

void _v_print(const char *str) {
    if (verbose_flag) {
        printf("%s", str);
    }
}

void Find(long long set_index, long long tag, _cache_line cache_line[S][E]) {
    _cache_line *hit = NULL;
    int i;
    
    for (i = 0; i < E; ++i) {
        _cache_line *tmp = &cache_line[set_index][i];
        if (tmp->valid == true && tmp->tag == tag) {
            hit = tmp;
            hit->timestamp = time;
            _v_print(" hit");
            hit_total++;
            break;
        }
    }

    if (hit == NULL) {
        _v_print(" miss");
        miss_total++;

        int lru_time = ~(1 << 31);
        int lru_index = -1;
        _cache_line *tmp = NULL;
        
        for (i = 0; i < E; i++) {
            int _timestamp = cache_line[set_index][i].timestamp;
            if (cache_line[set_index][i].valid){
                lru_index = _timestamp < lru_time ? i : lru_index;
                lru_time = _timestamp < lru_time ? _timestamp : lru_time;
                continue;
            };

            tmp = &cache_line[set_index][i];
            tmp->valid = true;
            tmp->timestamp = time;
            tmp->tag = tag;
            break;
        }

        if (tmp == NULL) {
            _v_print(" eviction");
            eviction_total++;

            tmp = &cache_line[set_index][lru_index];
            tmp->valid = true;
            tmp->timestamp = time;
            tmp->tag = tag;
        }
    }
}

int main(int argc, char *argv[]) {
    char c, line[30];

    if (argc != 9 && argc != 10) {
        usage(argv[0]);
    }
    
    while ((c = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        switch (c) {
            case 'v': // verbose flag
                verbose_flag = true;
                break;
            case 's': // Number of set index bits
                s = atoi(optarg);
                break;
            case 'E': // Number of lines per set
                E = atoi(optarg);
                break;
            case 'b': // Number of block offset bits
                b = atoi(optarg);
                break;
            case 't': // Trace file
                file_index = optind - 1;
                break;
            default:
                usage(argv[0]);
        }
    }

    if (s == 0 || E == 0 || b == 0 || file_index == 0) {
        usage(argv[0]);
    }

    if (!(infile = fopen(argv[file_index], "r"))) {
	    printf("%s: Error: Couldn't open %s\n", argv[0], argv[file_index]);
	    exit(8);
	}

    S = pow(2, s); //double -> long long

    _cache_line cache_line[S][E];
    int i, j;
    for (i = 0; i < S; i++) {
        for (j = 0; j < E; j++) {
            cache_line[i][j].valid = false;
            cache_line[i][j].timestamp = ~(1 << 31);
        }
    }

    char label;
    long long addr;
    int size;
    
    tag_mask = (tag_mask >> (b + s)) << (b + s);

    int tmp = s;
    while (tmp--) {
        set_mask <<= 1;
        set_mask |= 1;
    }
    set_mask <<= b;

    while (fscanf(infile, "%[^\n]%c", line, &c) != EOF) {
        if (line[0] == 'I') continue;

        _v_print(line);
        sscanf(line, " %c %llx,%d", &label, &addr, &size);
        
        long long set_index = (addr & set_mask) >> b;
        long long tag = (addr & tag_mask) >> (b + s);
        
        switch (line[1]) {
            case 'M':
                Find(set_index, tag, cache_line);
            case 'L':
            case 'S':
                Find(set_index, tag, cache_line);
                break;
            default :
                usage(argv[0]);
        }
        _v_print("\n");
        time++;
    }
    fclose(infile);
    printSummary(hit_total, miss_total, eviction_total);
    return 0;
}
