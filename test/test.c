#include <stdio.h>
// #include <tuple>
// #include <utility>
// #include <string>
// #include <iostream>
// #include <regex>
// #include <type_traits>

// using namespace std;

typedef unsigned char * byte_pointer;
void show_bytes(byte_pointer x, size_t len) {
    size_t i;
    for (i = 0; i < len; i++) {
        printf(" %.2x", x[i]);
    }
    printf("\n");
}


int main() {
    
    short ival = -32768;
    printf("%hd\n", ival + ival);
    return 0;
}