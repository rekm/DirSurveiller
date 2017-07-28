/**
 * This is an implementation of an adaptive radix tree
 * It is largely copied from existing reference material.
 * In particular the c++ code of the original authors of
 * The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases
 * by Viktor Leis, Alfons Kemper, Thomas Neumann
 *
 * The rest is from Armon Dadgars libart implementation,
 * which is a c library that already implements art in c. 
 *
 * https://github.com/armon/libart
 * */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


/* Dealing with diffrent architectures */
#ifdef __i386__
    #include <emmintrin.h>
#else
#ifdef __amd64__
    #include <emmintrin.h>
#endif
#endif



