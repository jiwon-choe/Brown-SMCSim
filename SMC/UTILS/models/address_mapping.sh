#!/bin/bash

if [ -z $1 ]; then print_err "address_mapping.sh needs an argument!"; fi
if   [ $1 == "VA.BA.RC.OF" ]; then P=(CH LB RC OF); export_array ADDRESS_MAPPING P;
elif [ $1 == "VA.RC.BA.OF" ]; then P=(CH RC LB OF); export_array ADDRESS_MAPPING P;
elif [ $1 == "BA.VA.RC.OF" ]; then P=(LB CH RC OF); export_array ADDRESS_MAPPING P;
elif [ $1 == "BA.RC.VA.OF" ]; then P=(LB RC CH OF); export_array ADDRESS_MAPPING P;
elif [ $1 == "RC.VA.BA.OF" ]; then P=(RC CH LB OF); export_array ADDRESS_MAPPING P;
elif [ $1 == "RC.BA.VA.OF" ]; then P=(RC LB CH OF); export_array ADDRESS_MAPPING P;
elif [ $1 == "LOW.INTLV" ];   then P=(RC LB CH OF); export_array ADDRESS_MAPPING P; # RC.BA.VA.OF
elif [[ $1 == S* ]]; then
    s=$1
    s="${s//"."/" "}"
    t=( $s )    # Get string tokens
    if [ -z ${t[4]} ]; then option=""; else option="_t"; fi
    P=( "S${t[1]}-${t[2]}/log${t[1]}_${t[2]}_${t[3]}${option}.sv" ); export_array ADDRESS_MAPPING P;
else
    print_err "Illegal address mapping! $1"
fi
