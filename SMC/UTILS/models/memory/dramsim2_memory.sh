#!/bin/bash

columns=`perl -l -e "print (2**($DRAM_column_size))"` # Number of columns per each row

echo -e "; blah blah
NUM_BANKS=$DRAM_BANKS_PER_VAULT
NUM_ROWS=$DRAM_ROWS_PER_BANK 
NUM_COLS=$columns
DEVICE_WIDTH=$DRAM_BUS_WIDTH

;in nanoseconds
;#define REFRESH_PERIOD 7800
REFRESH_PERIOD=`perl -l -e "print (1000*$DRAM_tREFI)"`
tCK=$DRAM_tCK ;

CL=$(div_ceil $DRAM_tCL $DRAM_tCK)
AL=0 ;
;AL=3; needs to be tRCD-1 or 0
;RL=(CL+AL)
;WL=(RL-1)
BL=$CH_BURST_LENGTH ;

tRAS=$(div_ceil $DRAM_tRAS $DRAM_tCK);
tRCD=$(div_ceil $DRAM_tRCD $DRAM_tCK);
tRRD=$(div_ceil $DRAM_tRRD $DRAM_tCK);
tRC=0 ;
tRP=$(div_ceil $DRAM_tRP $DRAM_tCK);
tCCD=0 ;
tRTP=$(div_ceil $DRAM_tRTP $DRAM_tCK);
tWTR=10 ;
tWR=$(div_ceil $DRAM_tWR $DRAM_tCK);
tRTRS=1;
tRFC=$(div_ceil $DRAM_tRFC $DRAM_tCK);
tFAW=62;
tCKE=3 ;
tXP=0  ;

tCMD=1 ;

IDD0=85
IDD1=100
IDD2P=7;
IDD2Q=40;
IDD2N=40;
IDD3Pf=30;
IDD3Ps=10;
IDD3N=55;
IDD4W=135;
IDD4R=135;
IDD5=215;
IDD6=7;
IDD6L=5;
IDD7=280;
Vdd=1.8 ; TODO: double check this"