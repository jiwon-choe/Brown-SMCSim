#ifndef DEFS_H_LINKEDLIST
#define DEFS_H_LINKEDLIST

#include "kernel_params.h"

typedef struct request_slot {
    uint16_t info_bits; // bit 0:   request valid bit, 
                       // bit 1,2: operation type bits
                       // bit 15:   return value
    uint16_t timestamp; 
    ulong_t parameter; 
} request_slot_t;



#define ADD_OP 0x4000      // 0x4000 == binary 0100 0000 0000 0000
#define REMOVE_OP 0x2000   // 0x2000 == binary 0010 0000 0000 0000
#define CONTAINS_OP 0x6000 // 0x6000 == binary 0110 0000 0000 0000

#define SET_OPERATION(_infobits, _op) { _infobits &= 0x1FFE; _infobits |= _op; }  // 0x1FFE == binary 0001 1111 1111 1110 (clear info from previous request, then set new operation) 
#define EXTRACT_OPERATION(_infobits)  (_infobits & 0x6000) // 0x6000 == binary 0110 0000 0000 0000

#define SET_RETVAL(_infobits, _retval)  (_infobits |= _retval)
#define GET_RETVAL(_infobits)  (_infobits & 0x0001)  // 0x0001 == binary 0000 0000 0000 0001

#define SET_REQUEST(_infobits)  (_infobits |= 0x8000)   // 0x8000 == binary 1000 0000 0000 0000
#define CHECK_VALID_REQUEST(_infobits)  (_infobits & 0x8000) 
#define CLEAR_REQUEST(_infobits)  (_infobits &= 0x7FFF) //   0x7FFF == binary 0111 1111 1111 1111 


#endif
