#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "app_utils.hh"     // This must be included before anything else
using namespace std;
#include "utils.hh"
#include "crypto_scrypt.h"



/************************************************/
// Main
int main(int argc, char *argv[])
{
    uint8_t kbuf[64];
    unsigned int i = 0;
    int ret = 0;
    char password_char[32] = PASSPHRASE;
    char salt_char[32] = SALT;
    char key_char[129] = KEY;
    uint8_t password[32];
    uint8_t salt[32];
    uint8_t key[64];
    char temp[2];

    PIMAPI *pim[PARALLEL_PARAM] = {0};//new PIMAPI(); /* Instatiate the PIM API */ 
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    for (i = 0; i < PARALLEL_PARAM; i++) {
        pim[i] = new PIMAPI();
        pim[i]->offload_kernel((char*)FILE_NAME);
    }
    cout << "scrypt on password: " << PASSPHRASE << endl;
    cout << "salt: " << SALT << endl;
    cout << "expected key: " << KEY << endl;

    cout << "CONVERTING TO APPROPRIATE FORMATS..." << endl;
    cout << "password:" << endl;
    for (i = 0; i < strlen(password_char); i++) {
        password[i] = (uint8_t)password_char[i];
        cout << (char)password[i];
    }
    cout << "\nsalt:" << endl;
    for (i = 0; i < strlen(salt_char); i++) {
        salt[i] = (uint8_t)salt_char[i];
        cout << (char)salt[i];
    }
    cout << "\nexpected key:" << endl;
    for (i = 0; i < 128; i+=2) {
        if ((key_char[i] >= 'a') && (key_char[i] <= 'f')) {
            key[i/2] = (key_char[i] - 'a' + 10)*16;
        } else {
            key[i/2] = (key_char[i] - '0')*16;
        }
        if ((key_char[i+1] >= 'a') && (key_char[i+1] <= 'f')) {
            key[i/2] += (key_char[i+1] - 'a' + 10);
        } else {
            key[i/2] += (key_char[i+1] - '0');
        }
        sprintf(temp, "%2x", key[i/2]);
        cout << temp;
    }
    cout << "\n" << endl;

    ____TIME_STAMP_PIMNO(0,0);
    for (i = 0; i < PARALLEL_PARAM; i++) {
        pim[i]->write_sreg(0,BLOCK_SIZE_PARAM); // r
        pim[i]->write_sreg(1,CPU_MEM_COST); // N
        pim[i]->write_sreg(2,64); // memory alignment
        pim[i]->write_sreg(3,i); // PIM ID
    }
    ret = crypto_scrypt(password, strlen(password_char), salt, strlen(salt_char), CPU_MEM_COST, BLOCK_SIZE_PARAM, PARALLEL_PARAM, kbuf, 64, pim);
    ____TIME_STAMP_PIMNO(0,1);
    if (ret != 0) {
        cout << "error in scrypt" << endl;
    } else {
        for (i = 0; i < 64; i++) {
            sprintf(temp, "%2x", kbuf[i]);
            cout << temp;
        }
        cout << "\n" << endl;
    }

    cout << "Exiting gem5 ..." << endl;
    pim[0]->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

