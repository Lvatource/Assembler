/* ============================================================== */
/* This file includes all code regarding the first pass over the  */
/* source code in the given .as files.                            */
/* ============================================================== */

#include "firstPass.h"
#include "utilityFunctions.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*Checks if a label name fits the label format. Prints an error message and return false if found errors and true otherwise*/
bool checkLabel(char* labelName, labelData* labels, int labelCount, operationData* operations, int lineCount);

/*Writes data to the data array for data storage guidance instructions. Updates DC accordingly*/
void writeDataFromGuidance(int guidanceNum, unsigned char** dataArray, int* DC, char* dataString);

/*Transforms operations and parameters to their corresponding binary code. Returns the four bytes of code as an int*/
int operationCode(operationData currentOperation, char* parameters);

/*Checks if the parameters of an operation fit its format. Prints an error message and return false if found errors and true otherwise*/
bool operationParameterCheck(int line, int IC, char* parameters, operationData currentOperation);

/*Checks if the parameters of a guidance instructions fit its format. Prints an error message and return false if found errors and true otherwise*/
bool checkGuidanceParam(int line, int guidanceNum, char* parameters);

/*Checks the general format of all parameters, commas, numbers, and spaces. Prints an error message and return false if found errors and true otherwise*/
bool checkParameterFormat(char* param, int line);


/*Runs the first pass of the assembler. It fills the data array, labels array, and code array as much as possible and checks the code for errors. Returns true if ran successfully, and false otherwise*/

bool firstPass(FILE* fp, labelData** labels, int* labelCount, unsigned char** dataArray, unsigned int** codeArray, int* IC, int* DC,int* JOpCounter, operationData* operations)
{
	char* line = malloc(MAXLINESTRLENGTH); /*Get Line*/
	char* ogLine = line; /*The original address of the line string*/
	int lineLength = 0;
	int lineCount = 1;
	bool isSuccessful = true;
	char c;
	bool isNewLine = false;
	while (true) /*Iterate till we get an EOF or end of line, or we find an error*/
	{
		c = getc(fp);
		if (isNewLine) {
			if (c == EOF) break;
			line = ogLine;
			lineCount++;
			lineLength = 0;
			isNewLine = false;
		}
		if (c != '\n' && c != EOF)
		{
			if (lineLength < MAXLINESTRLENGTH)
			{
				line[lineLength] = c;
				lineLength++;
			}
			else
			{
				printf("Error: Line %d: more than %d characters in line\n", lineCount, MAXLINESTRLENGTH); /*If line is too long:*/
				isSuccessful = false; /*Found an error*/
			}
		}
		else
		{
			/*Here we check if the line is a blank line*/
			int i;
			bool isEmptyLine = true;
			char firstChar = ' ';
			line[lineLength] = '\0';
			for (i = 0; i < lineLength; i++)
			{
				if (!isspace(line[i])) {
					isEmptyLine = false;
					firstChar = line[i];
					break;
				}
			}
			if (isEmptyLine == false && firstChar != ';') /*If the current line isn't an empty line or a comment:*/
			{
				labelData labelToAdd = {"",0,false,false,false,false}; /*Create a new instance of labelData*/
				char dataString[MAXLINESTRLENGTH] = "";
				char  temp[MAXLINESTRLENGTH] = "";
				char parameters[MAXLINESTRLENGTH] = "";
				int guidanceNum;
				char* operationName;
				int operationNumber;
				operationData currentOperation;
				char *labelName = malloc(MAXLABELSTRLENGTH + 2);
				bool isLabel = false;
				bool isLabelOk = true;
				int codeByte;
				int i = 0;
				if (scanStrAndMove(&line, "%33s", labelName) == 1)
				{
					while (labelName[i] != '\0') i++;
					if (labelName[i - 1] == ':') {
						labelName[i - 1] = '\0';
						isLabelOk = checkLabel(labelName, *labels, *labelCount, operations, lineCount);
						if (isLabelOk) /*If the label ended up being valid:*/
						 {
							isLabel = true;
							if ((*labelCount) % 10 == 0) {
								*labels = (labelData*)realloc(*labels, sizeof(labelData) * (*labelCount + 10)); /*Add label to label list*/
							}
							(*labelCount)++; /*Add one to the label counter*/
						}
					}
				}
				if (!isLabelOk) { /*If the label ended up being invalid:*/
					free(labelName);
					isNewLine = true;
					isSuccessful = false;
					continue; /*Move on*/
				}
				/*Add the label to the table, continue scanning until end of line*/
				operationName = malloc(10);
				if (isLabel) {
					scanStrAndMove(&line, "%s", operationName);
				}
				else {
					free(operationName);
					operationName = labelName;
				}
				guidanceNum = isGuidance(operationName + 1); /*Skip the . at the start of the insturction*/
				if (operationName[0] == '.' && guidanceNum >= 0) {/*If found a guidance insturction:*/
					if (guidanceNum == 3) {
						while (*line != '\"' && *line != '\0') line++;
						dataString[0] = line[0];
						if (line[0] != '\0') {
							int i = 0;
							while (i == 0 || (line[i] != '\"' && line[i] != '\0')) {
								i++;
								dataString[i] = line[i];
							}
						}
						
					}
					else
					{
						if (guidanceNum <= 2 && !checkParameterFormat(line, lineCount)) {
							free(labelName);
							isSuccessful = false;
							isNewLine = true;
							continue;
						}
						while (scanStrAndMove(&line, "%s", temp) > 0) {
							strcat(dataString, temp);
						}
					}
					if (checkGuidanceParam(lineCount, guidanceNum, dataString)){
						
						if (guidanceNum <= 3) {
							if (isLabel) {
								labelToAdd.symbol[0] = '\0'; labelToAdd.address = *(DC); labelToAdd.isEntry = false;
									labelToAdd.isExternal = false; labelToAdd.isData = true; labelToAdd.isCode = false;
									strcat(labelToAdd.symbol, labelName);
									(*labels)[*labelCount - 1] = labelToAdd;
							}
							writeDataFromGuidance(guidanceNum, dataArray, DC, dataString);
						}
						else {
							if (guidanceNum == 4) {
								free(labelName);
								isNewLine = true;
								continue;
							}
							if (guidanceNum == 5) {
								if (checkLabel(dataString, *labels, *labelCount, operations, lineCount)) {
									if ((*labelCount) % 10 == 0) {
										*labels = (labelData*)realloc((*labels), sizeof(labelData) * (*labelCount + 10));
									}
									(*labelCount)++;
									labelToAdd.symbol[0] = '\0'; labelToAdd.address = 0; labelToAdd.isEntry = false;
									labelToAdd.isExternal = true; labelToAdd.isData = false; labelToAdd.isCode = false;
									strcat(labelToAdd.symbol, dataString);
									(*labels)[*labelCount - 1] = labelToAdd;
									if (isLabel) printf("Warning: Line %d: label before extern is meaningless\n", lineCount);
								}

							}
						}
					}
					else {
						free(labelName);
						isNewLine = true;
						isSuccessful = false;
						continue;
					}
				}
				else {/*If it isnt a guidance insturction, it must be a normal instrction*/
					if (isLabel) {
						labelToAdd.symbol[0] = '\0'; labelToAdd.address = *(IC); labelToAdd.isEntry = false; 
						labelToAdd.isExternal = false; labelToAdd.isData = false; labelToAdd.isCode = true;
						strcat(labelToAdd.symbol, labelName);
						(*labels)[*labelCount-1] = labelToAdd;
					}
					operationNumber = operationNum(operations, operationName);
					if (operationNumber == -1) {
						printf("Error: Line %d: unknown word used as operation or guidance\n", lineCount);
						free(labelName);
						isSuccessful = false;
						isNewLine = true;
						continue;

					}
					currentOperation = operations[operationNumber];
					if (currentOperation.operationType == 'J') (*JOpCounter)++;
					if (!checkParameterFormat(line, lineCount)) {
						free(labelName);
						isSuccessful = false;
						isNewLine = true;
						continue;
					}
					while (scanStrAndMove(&line, "%s", temp) > 0) {
						strcat(parameters, temp);
					}
					codeByte = (*IC - 100);
					if (codeByte % 40 == 0) {
						*codeArray = realloc((*codeArray), codeByte + 40);
					}
					if (operationParameterCheck( lineCount,*IC, parameters, currentOperation)) {
						(*codeArray)[codeByte / 4] = operationCode(currentOperation, parameters);/*set the four bytes to thare corrosponding binary code*/
						*IC += 4;
					}
					else {
						free(labelName);
						isSuccessful = false;
						isNewLine = true;
						continue;
					}

				}
				if (isLabel) {
					free(operationName);
				}
				free(labelName);
				isNewLine = true;
			}
			else
			{
				isNewLine = true;
				continue;
			}
		}
		
	}
	free(ogLine);
	return isSuccessful; /*Return whether or not the line was free of errors*/
}
 
bool checkLabel(char* labelName, labelData* labels, int labelCount, operationData* operations, int lineCount)
{
	if (strlen(labelName) > MAXLABELSTRLENGTH - 1) /*If the label is too long:*/
	{
		printf("Error: Line %d: more than %d characters in a label\n", lineCount, MAXLABELSTRLENGTH); /*Print error message*/
		return false;
	}
	if (!isalpha(labelName[0])) /*If the label dosent start with an alphabetic character:*/
	{
		printf("Error: Line %d: label has to start with an alphabetic character\n", lineCount); /*Print error message*/
		return false;
	}
	if (labelNum(labels, labelCount, labelName) != -1) { /*If the labels name isn't original*/
		printf("Error: Line %d: a label with that name already exists\n", lineCount); /*Print error message*/
		return false;

	}
	if (operationNum(operations, labelName) != -1) { /*If the labels name is a protected key word*/
		printf("Error: Line %d: label name can't be an operation name\n", lineCount); /*Print error message*/
		return false;

	}
	if (isGuidance(labelName) != -1) { /*If the labels name is a protected key word*/
		printf("Error: Line %d: label name can't be a guidance word name\n", lineCount); /*Print error message*/
		return false;

	}
	return true;
}

void writeDataFromGuidance(int guidanceNum, unsigned char** dataArray, int* DC, char* dataString) {
	if (guidanceNum == 0) {
		int i = 0;
		int num = 0u;
		*dataArray = realloc(*dataArray, *DC + 40);
		while (moveAndScanInt(&dataString, "%hhd", &num) > 0) {
			(*dataArray)[*DC + i] = num;
			i++;
		}
		*DC += i;
	}
	if (guidanceNum == 1) {
		int i = 0;
		int num = 0u;
		*dataArray = realloc(*dataArray, *DC + 80);
		while (moveAndScanInt(&dataString, "%hd", &num) > 0) {
			*((short int*)&((*dataArray)[*DC + i])) = num;
			i += 2;
		}
		*DC += i;
	}
	if (guidanceNum == 2) {
		int i = 0;
		int  num;
		*dataArray = realloc(*dataArray, *DC + 160);
		while (moveAndScanInt(&dataString, "%d", (int*)&num) > 0) {
			*((int*)&((*dataArray)[*DC + i])) = num;
			i += 4;
		}
		*DC += i;
	}
	if (guidanceNum == 3) {
		int i;
		int sLength;
		sLength = strlen(dataString);
		if (sLength > 1) {
			dataString[sLength - 1] = '\0';
			dataString++;
			*dataArray = realloc(*dataArray, *DC + sLength);
			for (i = 0; i < sLength - 1; i++) {
				(*dataArray)[*DC + i] = dataString[i];
			}
			*DC += sLength - 1;
		}
	}
}

int operationCode(operationData currentOperation, char* parameters) {

	unsigned int retVal = 0u;
	if (currentOperation.operationType == 'R') {
		if (currentOperation.opcode == 0) {
			int registerArray[3];
			if (sscanf(parameters, "$%d,$%d,$%d", &registerArray[0], &registerArray[1], &registerArray[2]) > 0) {
				writeToBits(&retVal, 6, 10, currentOperation.funct);
				writeToBits(&retVal, 11, 15, registerArray[2]);
				writeToBits(&retVal, 16, 20, registerArray[1]);
				writeToBits(&retVal, 21, 25, registerArray[0]);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}
		}
		else {

			int registerArray[2];
			if (sscanf(parameters, "$%d,$%d", &registerArray[0], &registerArray[1]) > 0) {
				writeToBits(&retVal, 6, 10, currentOperation.funct);
				writeToBits(&retVal, 21, 25, registerArray[0]);
				writeToBits(&retVal, 11, 15, registerArray[1]);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}
		}
	}

	if (currentOperation.operationType == 'I') {
		if (currentOperation.opcode < 15) {
			int paramArray[2];
			short int immed = 0;
			if (sscanf(parameters, "$%d,%hd,$%d", &paramArray[0], &immed, &paramArray[1]) > 0) {
				writeToBits(&retVal, 0, 15, immed);
				writeToBits(&retVal, 16, 20, paramArray[1]);
				writeToBits(&retVal, 21, 25, paramArray[0]);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}

		}
		if (currentOperation.opcode < 19) {
			int paramArray[2];
			if (sscanf(parameters, "$%d,$%d", &paramArray[0], &paramArray[1]) > 0) {
				writeToBits(&retVal, 16, 20, paramArray[1]);
				writeToBits(&retVal, 21, 25, paramArray[0]);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}
		}
		else {
			int paramArray[2];
			short int immed = 0;
			if (sscanf(parameters, "$%d,%hd,$%d", &paramArray[0], &immed, &paramArray[1]) > 0) {
				writeToBits(&retVal, 0, 15, immed);
				writeToBits(&retVal, 16, 20, paramArray[1]);
				writeToBits(&retVal, 21, 25, paramArray[0]);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}
		}
	}
	if (currentOperation.operationType == 'J') {
		if (currentOperation.opcode == 30) {
			int regNum = 0;
			if (sscanf(parameters, "$%d", &regNum) > 0) {
				writeToBits(&retVal, 25, 25, 1);
				writeToBits(&retVal, 0, 24, regNum);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}
			else {
				writeToBits(&retVal, 25, 25, 0);
				writeToBits(&retVal, 26, 31, currentOperation.opcode);
				return retVal;
			}
		}
		if (currentOperation.opcode <= 32) {
			writeToBits(&retVal, 25, 25, 0);
			writeToBits(&retVal, 26, 31, currentOperation.opcode);
			return retVal;
		}
		if (currentOperation.opcode == 63) {
			writeToBits(&retVal, 26, 31, currentOperation.opcode);
			writeToBits(&retVal, 0, 25, 0);
			return retVal;
		}
	}
	return retVal;
}

bool operationParameterCheck(int line, int IC, char* parameters, operationData currentOperation)
{
	int i;
	int paramNum = 0;
	int num = 0;
	int countNumLengths = 0;
	char* ogParam = parameters;
	char* og_xParam = ogParam;
	if (currentOperation.operationType == 'R')
	{
		if (currentOperation.opcode == 0)
		{
			if (isdigit(parameters[0]))
			{
				printf("Line %d: invalid register defining, no $\n", line);
				return false;
			}
			for (i = 0; i <= 2; i++)
			{
				if (moveAndScanInt(&parameters, "%d", &num) > 0) paramNum++;
				if (paramNum > i)
				{
					if (num > 31 || num < 0)
					{
						printf("Line %d: invalid register number\n", line);
						return false;
					}
					if (*(parameters) != COMMA && *(parameters) != '\0')
					{
						printf("Line %d: invalid register defining, no comma\n", line);
						return false;
					}
					og_xParam = parameters;
					parameters--;
					while (isdigit(*parameters)) parameters--;
					if (*(parameters) != DOLLAR)
					{
						printf("Line %d: invalid register defining, no $\n", line);
						return false;
					}
					parameters = og_xParam;
					countNumLengths += numberLength(num);
				}
			}
			if (paramNum < 3)
			{
				printf("Line %d: not enough parameters in %s operation\n", line, currentOperation.operationName);
				return false;
			}
			i = strlen(ogParam);
			if (i > (6 + countNumLengths))
			{
				printf("Line %d: invalid char in %s operation parameters\n", line, currentOperation.operationName);
				return false;
			}
			return true;
		}
		else
		{
			if (isdigit(parameters[0]))
			{
				printf("Line %d: invalid register defining, no $\n", line);
				return false;
			}
			for (i = 0; i <= 1; i++)
			{
				if (moveAndScanInt(&parameters, "%d", &num) > 0) paramNum++;
				if (paramNum > i)
				{
					if (num > 31 || num < 0)
					{
						printf("Line %d: invalid register number\n", line);
						return false;
					}
					if (*(parameters) != COMMA && *(parameters) != '\0')
					{
						printf("Line %d: invalid register defining, no comma\n", line);
						return false;
					}
					og_xParam = parameters;
					parameters--;
					while (isdigit(*parameters)) parameters--;
					if (*(parameters) != DOLLAR)
					{
						printf("Line %d: invalid register defining, no $\n", line);
						return false;
					}
					parameters = og_xParam;
					countNumLengths += numberLength(num);
				}
			}
			if (paramNum < 2)
			{
				printf("Line %d: not enough parameters in %s operation\n", line, currentOperation.operationName);
				return false;
			}
			i = strlen(ogParam);
			if (i > (4 + countNumLengths))
			{
				printf("Line %d: invalid char in %s operation parameters\n", line, currentOperation.operationName);
				return false;
			}
			return true;
		}
	}

	if (currentOperation.operationType == 'I')
	{
		if (currentOperation.opcode < 15 || currentOperation.opcode > 18)
		{
			if (isdigit(parameters[0]))
			{
				printf("Line %d: invalid register defining, no $\n", line);
				return false;
			}

			for (i = 0; i <= 2; i++)
			{
				if (moveAndScanInt(&parameters, "%d", &num) > 0) paramNum++;
				if (paramNum > i)
				{
					if (paramNum == 2 && (num > 32677 || num < -32678))
					{
						printf("Line %d: invalid inmed number\n", line);
						return false;
					}
					if (paramNum != 2 && (num > 31 || num < 0))
					{
						printf("Line %d: invalid register number\n", line);
						return false;
					}
					if (*(parameters) != COMMA && *(parameters) != '\0')
					{
						printf("Line %d: invalid register defining, no comma\n", line);
						return false;
					}
					og_xParam = parameters;
					parameters--;
					while (isdigit(*parameters)) parameters--;
					if (*(parameters) != DOLLAR && paramNum != 2) 
					{
						printf("Line %d: invalid register defining, no $\n", line);
						return false;
					}
					parameters = og_xParam;
					countNumLengths += numberLength(num);
				}
			}
			if (paramNum < 3)
			{
				printf("Line %d: not enough parameters in %s operation\n", line, currentOperation.operationName);
				return false;
			}
			i = strlen(ogParam);
			if (i > (6 + countNumLengths))
			{
				printf("Line %d: invalid char in %s operation parameters\n", line, currentOperation.operationName);
				return false;
			}
			return true;
		}

		else
		{
			if (isdigit(parameters[0]))
			{
				printf("Line %d: invalid register defining, no $\n", line);
				return false;
			}

			for (i = 0; i <= 1; i++)
			{
				if (moveAndScanInt(&parameters, "%d", &num) > 0) paramNum++;
				if (paramNum > i)
				{
					if (num > 31 || num < 0)
					{
						printf("Line %d: invalid register number\n", line);
						return false;
					}

					if (*(parameters) != COMMA && *(parameters) != '\0')
					{
						printf("Line %d: invalid register defining, no comma\n", line);
						return false;
					}
					og_xParam = parameters;
					parameters--;
					while (isdigit(*parameters)) parameters--;
					if (*(parameters) != DOLLAR)
					{
						printf("Line %d: invalid register defining, no $\n", line);
						return false;
					}
					parameters = og_xParam;
					countNumLengths += numberLength(num);
				}
			}
			if (paramNum < 2)
			{
				printf("Line %d: not enough parameters in %s operation\n", line, currentOperation.operationName);
				return false;
			}

			i = strlen(ogParam);
			if (!isalpha(parameters[1]))
			{
				printf("Line %d: no label in %s operation parameters\n", line, currentOperation.operationName);
				return false;
			}
			else
			{
				if (i > (6 + countNumLengths + strlen(parameters)))
				{
					printf("Line %d: invalid char in %s operation parameters\n", line, currentOperation.operationName);
					return false;
				}
				return true;
			}

		}
		return true;
	}

	if (currentOperation.operationType == 'J')
	{
		if (currentOperation.opcode == 30)
		{
			if (moveAndScanInt(&parameters, "%d", &num) > 0)
			{
				if (!isalpha(ogParam[0]) && ogParam[0] != DOLLAR)
				{
					printf("Line %d: invalid label or register defining in %s operation\n", line, currentOperation.operationName);
					return false;
				}
				if (!isalpha(ogParam[0]))
				{
					if (num > 31 || num < 0)
					{
						printf("Line %d: invalid register number\n", line);
						return false;
					}
				}
				if (!isspace(*parameters) && *parameters != '\0') {
					printf("Line %d: unknown chars in operation parameters\n", line);
					return false;
				}
			}
			else
			{
				if (!isalpha(ogParam[0]))
				{
					printf("Line %d: invalid label defining\n", line);
					return false;
				}
			}
			return true;
		}
		if (currentOperation.opcode <= 32)
		{
			if (!isalpha(ogParam[0]))
			{
				printf("Line %d: invalid label defining\n", line);
				return false;
			}
		}
		if (currentOperation.opcode == 63)
		{
			if (parameters[0] != '\0')
			{
				printf("Line %d: stop function should not have parameters\n", line);
				return false;
			}
		}
	}
	return true;
}

bool checkGuidanceParam(int line, int guidanceNum, char* parameters)
{
	int numOfCommas = 0;
	int num;
	int countParamLengths = -1;
	int countParam = 0;
	int parametersLength = strlen(parameters);
	if (guidanceNum < 0)
	{
		return false;
	}
	if (guidanceNum == 0)
	{
		while (moveAndScanInt(&parameters, "%d", &num) > 0)
		{
			if (num > 127 || num < -128)
			{
				printf("Line %d: invalid parameter number, should be between -128 and 127\n", line);
				return false;
			}
			if (*parameters != COMMA && *parameters != '\0')
			{
				printf("Line %d: invalid parameter defining, no comma\n", line);
				return false;
			}
			if (*parameters == COMMA) numOfCommas++;
			countParamLengths += (numberLength(num) + 1);
			countParam++;
		}
		if (countParam < 1)
		{
			printf("Line %d: invalid Parameters in function\n", line);
			return false;
		}
		if (countParamLengths < parametersLength)
		{
			printf("Line %d: invalid char in operation parameters\n", line);

			return false;
		}
		if (countParam != numOfCommas + 1)
		{
			printf("Line % d: invalid parameter defining, wrong use of commas\n", line);

			return false;
		}
		return true;
	}
	if (guidanceNum == 1)
	{
		while (moveAndScanInt(&parameters, "%d", &num) > 0)
		{
			if (num > 32767 || num < -32768)
			{
				printf("Line %d: invalid parameter number, should be between -32768 and 32767\n", line);
				return false;
			}
			if (*parameters != COMMA && *parameters != '\0')
			{
				printf("Line %d: invalid parameter defining, no comma\n", line);
				return false;
			}
			if (*parameters == COMMA) numOfCommas++;
			countParamLengths += (numberLength(num) + 1);
			countParam++;
		}
		if (countParam < 1)
		{
			printf("Line %d: invalid Parameters in function\n", line);
			return false;
		}
		if (countParamLengths < parametersLength)
		{
			printf("Line %d: invalid char in operation parameters\n", line);

			return false;
		}
		if (countParam != numOfCommas + 1)
		{
			printf("Line % d: invalid parameter defining, wrong use of commas\n", line);

			return false;
		}
		return true;
	}

	if (guidanceNum == 2)
	{
		double longNum;
		while (sscanf(parameters, "%lf", &longNum) > 0|| sscanf(parameters, ",%lf", &longNum) > 0)
		{
			if (longNum > 2147483647.0 || longNum < -2147483648.0)
			{
				printf("Line %d: invalid parameter number, should be between -2147483648 and 2147483647\n", line);
				return false;
			}
			moveAndScanInt(&parameters, "%d", &num);
			if (*parameters != COMMA && *parameters != '\0')
			{
				printf("Line %d: invalid parameter defining, no comma\n", line);
				return false;
			}
			if (*parameters == COMMA) numOfCommas++;
			countParamLengths += (numberLength(num) + 1);
			countParam++;
		}
		if (countParam < 1)
		{
			printf("Line %d: invalid Parameters in function\n", line);
			return false;
		}
		if (countParamLengths < parametersLength)
		{
			printf("Line %d: invalid char in operation parameters\n", line);

			return false;
		}
		if (countParam != numOfCommas + 1)
		{
			printf("Line % d: invalid parameter defining, wrong use of commas\n", line);

			return false;
		}
		return true;
	}
	if (guidanceNum == 3)
	{
		if (parameters[0] != '"')
		{
			printf("Line %d: invalid string defining, no \" at the beginning of the string definition\n", line);

			return false;
		}
		if (parameters[strlen(parameters) - 1] != '"')
		{
			printf("Line %d: invalid string defining, no \" at the end of the string definition\n", line);

			return false;
		}
		return true;
	}
	if (guidanceNum == 4 || guidanceNum == 5)
	{
		if (!isalpha(parameters[0]))

		{

			printf("Line %d: invalid label defining\n", line);

			return false;

		}
		return true;
	}
	return true;
}

bool checkParameterFormat(char* param, int line)
{
	bool isNum = true;
	int idx = 0;
	while (param[idx] != '\0' && param[idx] != EOF) {
		bool numDone = true;
		if (param[idx] == COMMA) isNum = true;
		if ((isdigit(param[idx]) || param[idx] == DOLLAR) && isNum == false) {
			printf("Line %d: invalid parameter format\n", line);
			return false;
		}
		
		while (isdigit(param[idx]) || param[idx] == DOLLAR) {
			isNum = false;
			numDone = false;
			idx++;
		}

		if(numDone) idx++;
	}
	return true;
}