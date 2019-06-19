#ifndef DEFS_H_SKIPLIST
#define DEFS_H_SKIPLIST

#include "kernel_params.h"

typedef struct request_slot {
    uint8_t info_bits; // bit 0:   request valid bit, 
                       // bit 1,2: operation type bits
                       // bit 7:   return value
    uint8_t random_level;
    uint16_t timestamp; 
    ulong_t parameter; 
} request_slot_t; // 8+8+16+32 bits = 1+1+2+4 bytes = 8 bytes


#define MAX_LEVEL 30

#define ADD_OP 0x40      // 0x40 == binary 0100 0000
#define REMOVE_OP 0x20   // 0x20 == binary 0010 0000
#define CONTAINS_OP 0x60 // 0x60 == binary 0110 0000

#define SET_OPERATION(_infobits, _op) { _infobits &= 0x1E; _infobits |= _op; }  // 0x1E == binary 0001 1110 (clear info from previous request, then set new operation) 
#define EXTRACT_OPERATION(_infobits)  (_infobits & 0x60) // 0x60 == binary 0110 0000 

#define SET_RETVAL(_infobits, _retval)  (_infobits |= _retval)
#define GET_RETVAL(_infobits)  (_infobits & 0x01)  // 0x01 == binary 0000 0001

#define SET_REQUEST(_infobits)  (_infobits |= 0x80)   // 0x80 == binary 1000 0000
#define CHECK_VALID_REQUEST(_infobits)  (_infobits & 0x80) 
#define CLEAR_REQUEST(_infobits)  (_infobits &= 0x7F) //   0x7F == binary 0111 1111 


#endif
