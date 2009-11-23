#ifndef TODO_H
#define TODO_H

#include "eprintf.h"

#define TODO() do { EPRINTF(0, "TODO.\n"); exit(1); } while (1)

#endif  // #ifndef TODO_H
