
/**
 * removes spaces at the beginning of a line
 **/
char *strRemoveSpsSol(char *dest, const char *str);

int digitsInLine(const char *str, int str_s);

/**
 * receives a fileName with path and returns only the fileName
 **/
char *getFileName(char *destiny, const char *fileWithPath);

/**
 *  in a string with parameter in the format -param=value  return the param part
 * */
char *getParamName(char *target, const char *str);

/**
 *  in a string with parameter in the format -param=value  return the value part
 * */
char *getParamValue(char *target, const char *str);

/* converts all chacarters to uppercase */
void strAllToUpper(char *str);

/* removes spaces and other meaningless characthers from end of line */
char *strRemoveSpsEol(char *str);

/* char is invisible:  if char = ' ' or '\n' or '\t' or '\r' */
char charIsInvisible(const char c);

int splitString(char **columns, const char *str, const char delimiter,
  const int maxColumns, const char multDel);

/* to align at right */
void strFillSpacesLeft(char *dest, const char *str, int n);

/* to align at left */
void strFillSpacesRight(char *dest, const char *str, int n);

/* to align at center */
void strFillSpaces_both(char *dest, const char *str, int n);

/* remove repeated spacing characters (e.g. spaces of tabs ) */
char *strRemoveDblSpaces(char *str);

/* clear all spaces in a string */
char *strClearSpaces(char *str);

/* count occurrences of a given char */
int countChar(const char *str, const char c);

/* first occurrence of a given char */
int firstOccurrence(const char *str, const char c);

/* last occurrence of a given char */
int lastOccurrence(const char *str, const char c);

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
