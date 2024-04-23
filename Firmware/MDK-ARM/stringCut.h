#ifndef STRINGCUT_H
#define STRINGCUT_H <.h>
#include <string.h>
#include <stdio.h>
char* stringCut(char* str, char* delimiter, int field)
{
char* token = strtok(str, delimiter);
int count = 1;
while(token != NULL)
{
if(count == field)
{
return token;
}
token = strtok(NULL, delimiter);
count++;
}
return NULL;
}
#endif