#pragma once

#include <stddef.h>

#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"

// prints to stderr a message prefixed with "error: " and exit the program
// the arguments are the same as for printf
void err(const char *fmt, ...);

// allocates memory using malloc
// if succeeds, it returns the allocated memory, else it prints an error message
// and exit the program
void *safe_alloc(size_t n_bytes);

// loads a text file in a dynamically allocated memory and returns it
// on error, prints a message and exit the program
char *load_file(const char *file_name);

char *extract(const char *begin, const char *end);
