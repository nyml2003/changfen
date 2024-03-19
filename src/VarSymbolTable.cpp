#include <string.h>
#include <stdlib.h>

#include "VarSymbolTable.hpp"

char* varSymbolTable[VarSymbolTableSize];
int varSymbolCnt = 0;

int var(char *ident) {
    for (int i = 0; i < varSymbolCnt; ++i) {
        if (!strcmp(varSymbolTable[i], ident)) {
            return i;
        }
    }
    if (varSymbolCnt >= VarSymbolTableSize) {
        return 2;
    }
    int l = strlen(ident);
    char *tmp = NULL;
    if (!(tmp = (char *)malloc(l + 1))) {
        return 3;
    }
    strcpy(tmp, ident);
    varSymbolTable[varSymbolCnt] = tmp;
    return varSymbolCnt++;
}