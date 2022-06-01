/* ============================================================== */
/* This file includes all code regarding the intialization of the */
/* program, notably the main() fucntion.                          */
/* ============================================================== */

#include "firstPass.h"
#include "secondPass.h"
#include "createOutputFiles.h"
#include "utilityFunctions.h"
#include "assemblyData.h"

#include <stdlib.h>

/*This function recieves a string, and returns true if it ends with .as, or false otherwise. */
bool fileApproved(char* fileName) {
	int i = 0;
	while (fileName[i] != '\0') i++;
	if (i >= 5 && fileName[i - 1] == 's' && fileName[i - 2] == 'a' && fileName[i - 3] == '.') return true;
	printf("File isn't a .as file\n");
	return false;
}

int main(int argc, char *argv[])
{
	int fileIndex; /*We index each file*/
	for (fileIndex = 1; fileIndex < argc; fileIndex++){ /*For each file given to the assembler, iterate:*/
		labelData* labels = NULL;
		int labelCount = 0;
		unsigned int* codeArray = malloc(10);
		unsigned char* dataArray = malloc(10);
		int IC = 100;
		int DC = 0;
		FILE* fp = NULL;
		int JOpCounter = 0; /*This variable is used to store the amount of J operation lines received*/
		char* fileName = argv[fileIndex];
		int extArrayLength = 0;
		extUse* extArray; /*Will be used later to make the .ext file*/
		fp = fopen(fileName, "r"); /*Open current file*/
		if(!fileApproved(fileName) || fp == NULL) /*If the file opening failed:*/
		{
			printf("Error, couldn't open the file with the name %s\n", fileName); /*Print error message*/
			continue; /*Move on to the next file*/
		}
		if (firstPass(fp, &labels, &labelCount, &dataArray, &codeArray, &IC, &DC, &JOpCounter, operationsArray)) { /*If no error were found during the first pass:*/
			extArray = calloc(JOpCounter, sizeof(extUse)); /*Allocate space for the .extern labels*/
			fclose(fp); 
			fp = fopen(fileName, "r"); /*We close and open the file to start reading it from the beginning again*/
			if (secondPass(fp, labels, labelCount, codeArray, &IC, &DC, extArray, &extArrayLength, operationsArray)) { /*If no error were found during the second pass:*/
				if (!createObject(codeArray, dataArray, IC, DC, fileName)) { /*If the .ob file creation failed:*/
					printf("Error, couldn't create object file\n"); /*Print error message*/
				}
				else {
					printf("Object file created\n"); /*Print confirmation message*/
				}
				if (!createEnt(labels, labelCount, fileName)) { /*If the .ent file creation failed:*/
					printf("Error, couldn't create entries file\n"); /*Print error message*/
				}
				else {
					printf("Entries file created\n"); /*Print confirmation message*/
				}
				if (!createExt(extArray, extArrayLength, fileName)) { /*If the .ext file creation failed:*/
					printf("Error, couldn't create externals file\n"); /*Print error message*/
				}
				else {
					printf("Externals file created\n"); /*Print confirmation message*/
				}
			}
			free(extArray); /*Free allocated space*/
		}
		free(codeArray); /*Free allocated space*/
		free(dataArray); /*Free allocated space*/
		fclose(fp); /*Close orignal .as source file*/
	}
	return 0; /*End of program*/
}