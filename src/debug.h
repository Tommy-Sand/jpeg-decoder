#define DEBUG 0

#define debug_print(fmt, ...) \
    do { \
        if (DEBUG) \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
    } while (0)
