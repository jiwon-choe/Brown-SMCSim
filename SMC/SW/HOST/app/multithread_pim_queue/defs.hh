#ifndef DEFS_H_LINKEDLIST
#define DEFS_H_LINKEDLIST

#include "kernel_params.h"

typedef struct request_slot {
    uint16_t info_bits; // bit 15: request valid bit, bit 14: PIM valid bit, bit 0: operation type bit
    uint16_t timestamp; 
    ulong_t parameter; 
} request_slot_t;


#define SET_ENQ(_infobits) (_infobits |= 0x8000) // binary 1000 0000 0000 0000
#define SET_DEQ(_infobits) (_infobits &= 0x7fff) // binary 0111 1111 1111 1111
#define EXTRACT_OP(_infobits) (_infobits & 0x8000) // binary 1000 0000 0000 0000
#define SET_REQUEST(_infobits) (_infobits |= 0x0003) // binary 0000 0000 0000 0011
#define CLEAR_PIM_VALID(_infobits) (_infobits &= 0xfffd) // binary 1111 1111 1111 1101
#define CHECK_VALID_REQUEST(_infobits) (_infobits & 0x0001) // binary 0000 0000 0000 0001
#define CHECK_VALID_PIM(_infobits) (_infobits & 0x0002) //binary 0000 0000 0000 0010
#define CLEAR_REQUEST(_infobits) (_infobits &= 0xfffe) // binary 1111 1111 1111 1110
#define CLEAR_REQUEST_PIM_VALID(_infobits) (_infobits &= 0xfffc) // binary 1111 1111 1111 1100


#endif
