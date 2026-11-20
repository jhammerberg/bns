// The real base/src/main.c, with main() renamed so the sim can run it on its
// own thread. Nothing here is a copy of firmware code.
#define main firmware_main
#include "main.c"
