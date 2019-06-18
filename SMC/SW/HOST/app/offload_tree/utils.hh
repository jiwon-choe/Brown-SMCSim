#ifndef UTILS_H_TREE
#define UTILS_H_TREE

unsigned long pow2(unsigned value)
{
    unsigned long res=1;
    for ( unsigned v=0; v<value; v++ )
        res *= 2;
    return res;
}

#endif