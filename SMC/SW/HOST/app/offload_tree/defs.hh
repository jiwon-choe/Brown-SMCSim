#ifndef DEFS_H_TREE
#define DEFS_H_TREE

typedef struct nodes
{
    ulong_t key;
    struct nodes* left;
    struct nodes* right;
} node;

#endif