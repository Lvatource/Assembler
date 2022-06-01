/* ============================================================== */
/* This file includes all code regarding the second pass over the */
/* source code in the given .as files.                            */
/* ============================================================== */

#include "secondPass.h"
#include "utilityFunctions.h"
#include <string.h>

/*Completes the binary code that could not be written in the first pass and checks the parameters for errors.
This function can return multiple values: -1 if an error was found, 0 if it couldnt write, 1 if it wrote code, or   2 if the label used is external*/

int operationLabelCode(operationData currentOperation, char* parameters, unsigned int* codeArray, int lineNum, int IC, labelData* labels, int labelCount);

/*runs the second pass of the assembler. it fills the parts of data array that could not be written in the first pass and checks the code for errors.
if successful it returns true*/

bool secondPass(FILE* fp, labelData* labels, int labelCount, unsigned int* codeArray, int* IC, int* DC, extUse* extArray,int* extArrayLength, operationData* operations)
{
	int thisLine = 0;
	char temp[MAXLINESTRLENGTH] = "";
	char parameters[MAXLINESTRLENGTH] = "";
	char oglineStr[MAXLINESTRLENGTH];
	char* lineStr;
	char opScanStr[MAXLINESTRLENGTH];
	char* opName;
	char labelName[MAXLABELSTRLENGTH];
	int i = 0;
	int linesWithLabels = 0;
	operationData currentOperation;
	bool retVal = true;
	int currentIC = 100;
	int lineLength;
	int opNum;
	int opReturn;
	for (i = 0; i < labelCount; i++) { /*For each label, we update it's addresss*/
		if (labels[i].isData) {
			labels[i].address += *IC;
		}
	}
	while (fgets(oglineStr, 80, fp) != NULL) { /*Get next line*/
		temp[0] = '\0';
		parameters[0] = '\0';
		thisLine++;
		lineLength = strlen(oglineStr);
		if(oglineStr[lineLength -1] == '\n') oglineStr[lineLength -1] = '\0';
		lineStr = oglineStr;
		scanStrAndMove(&lineStr, "%s", opScanStr);
		if (strcmp(opScanStr, ".entry") == 0) {  /*Update the entry labels*/
			scanStrAndMove(&lineStr, "%s", labelName);
			labels[labelNum(labels, labelCount, labelName)].isEntry = true;
			continue;
		}
		if (opScanStr[strlen(opScanStr) - 1] == ':') {
			scanStrAndMove(&lineStr, "%s", opScanStr);
		}
		opName = opScanStr;
		opNum = operationNum(operations, opName);
		currentOperation = operations[opNum];
		if (currentOperation.opcode >= 15) {	/*Write the missing code from first pass*/
			while (scanStrAndMove(&lineStr, "%s", temp) > 0) {
				strcat(parameters, temp);
			}
			opReturn = operationLabelCode(currentOperation, parameters, codeArray, thisLine, currentIC, labels, labelCount);
			if (opReturn == -1) {
				retVal = false;
					
			}
			else if (opReturn == 2) {
				extArray[*extArrayLength].IC = currentIC;
				strcpy(extArray[*extArrayLength].label, parameters);
				linesWithLabels++;
				(*extArrayLength)++;
					
			}
			else {
				linesWithLabels += opReturn;
			}
		}
		if (opNum >= 0) currentIC += 4;
	}
	return retVal;
}

int operationLabelCode(operationData currentOperation, char* parameters, unsigned int* codeArray, int lineNum, int IC, labelData* labels, int labelCount) {
	
	labelData paramLabel;
	int opPos = (IC - 100) / 4;
	int labelNumber;
	if (currentOperation.operationType == 'J') { /*If the operation is of type J:*/
		if (currentOperation.opcode == 30) { /*If the operation is jmp*/
			int regNum = 0;
			if (sscanf(parameters, "$%d", &regNum) > 0) {
				return 0;
			}
			else {
				labelNumber = labelNum(labels, labelCount, parameters);
				if (labelNumber == -1) {
					printf("Error: Line %d: unknown label used as a parameter for J operation\n", lineNum);
					return -1;
				}
				paramLabel = labels[labelNumber];
				if (paramLabel.isExternal) {
					return 2;
				}
				writeToBits(&codeArray[opPos], 0, 24, paramLabel.address);
				return 1;
			}

		}
		if (currentOperation.opcode <= 32) { /*If the operation is la or call*/
			labelNumber = labelNum(labels, labelCount, parameters);
			if (labelNumber == -1) {
				printf("Error: Line %d: unknown label used as a parameter for J operation\n", lineNum);
				return -1;
			}
			paramLabel = labels[labelNumber];
			if (paramLabel.isExternal) {
				return 2;
			}
			writeToBits(&codeArray[opPos], 0, 24, paramLabel.address);
			return 1;
		}
	}
	if (currentOperation.operationType == 'I') { /*If the operation is of type I:*/
		if (currentOperation.opcode < 19) { /*If the operation isn't related to memory loading:*/
			int commaCounter = 0;
			short int distanceToLabel;
			while (commaCounter < 2) { /*Here we search for the different parameters we were given*/
				if (*parameters == COMMA) commaCounter++;
				parameters++;
			}
			labelNumber = labelNum(labels, labelCount, parameters);
			if (labelNumber == -1) {
				printf("Error: Line %d: unknown label used as a parameter for I operation\n", lineNum);
				return -1;
			}
			paramLabel = labels[labelNumber];
			if (paramLabel.isExternal) {
				printf("Error: Line %d: external label cannot be used as a parameter for I operation\n", lineNum);
				return -1;
			}
			distanceToLabel = (short int)(paramLabel.address - IC);
			writeToBits(&codeArray[opPos], 0, 15, distanceToLabel);
			return 1;
		}
	}
	return 0;
}