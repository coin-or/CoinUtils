// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinRead_H
#define CoinRead_H

#include <string>

/// Simple read stuff
std::string CoinReadNextField(FILE *ReadCommand, int ReadMode);
std::string CoinReadGetCommand(int argc, const char *argv[],
			       FILE *ReadCommand, int ReadMode);
std::string CoinReadGetString(int argc, const char *argv[],
			      FILE *ReadCommand, int ReadMode);
// valid 0 - okay, 1 bad, 2 not there
int CoinReadGetIntField(int argc, const char *argv[],int * valid,
			FILE *ReadCommand, int ReadMode);
double CoinReadGetDoubleField(int argc, const char *argv[],int * valid,
			      FILE *ReadCommand, int ReadMode);
void CoinReadPrintit(const char * input);

#endif
