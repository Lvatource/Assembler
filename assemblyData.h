
#ifndef ASSEMBLY_DATA_H
#define ASSEMBLY_DATA_H

#include <stdbool.h>

#define NUMOFOPERATIONS 27
#define NUMOFGUIDANCEWORDS 6
#define MAXLINESTRLENGTH 81
#define MAXLABELSTRLENGTH 32
#define DOLLAR '$'
#define COMMA ','

typedef struct /*This struct will be used to store all info relevant for a given operation- Name, type, funct (if exists) and opcode*/
{
	char operationName[5];
	char operationType;
	int funct;
	int opcode;

} operationData;

typedef struct /*This struct will be used to store all info relevant for a given label- Name, address, and whether or not it's an entry/external/data/code label*/
{
	char symbol[32];
	int address;
	bool isEntry;
	bool isExternal;
	bool isData;
	bool isCode;

} labelData;

typedef struct /*A struct to hold a use of an external label*/
{
	char label[32];
	int IC;

} extUse;

operationData operationsArray[NUMOFOPERATIONS];

#endif
