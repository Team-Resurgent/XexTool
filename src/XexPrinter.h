// 
// XexPrinter.h
// 
// prints xex info to iostream
// if no iostream is specified, info is printed to stdout
// 

#ifndef _XEX_PRINTER_H_
#define _XEX_PRINTER_H_

#include <stdio.h>
#include "XexImageEntryTypes.h"

class Xex;

class XexPrinter
{
public:
	XexPrinter();
	~XexPrinter();
	
	// print shortened info to iostream
	void printShort(const Xex& xex, FILE* stream = stdout) const;
	
	// print extended info to iostream
	void printAll(const Xex& xex, FILE* stream = stdout) const;
	
private:
	void printGameRatings(const GameRatings& ratings, FILE* stream) const;
};

#endif // _XEX_PRINTER_H_

