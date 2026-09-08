// XGetopt.h  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XGETOPT_H
#define XGETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNICODE

extern int optind, opterr;
extern char *optarg;

int getopt(int argc, char *argv[], char *optstring);

#else // UNICODE

extern int optind, opterr;
extern wchar_t *optarg;

int wgetopt(int argc, wchar_t *argv[], wchar_t *optstring);

#endif // UNICODE

#ifdef __cplusplus
}
#endif

#endif // XGETOPT_H

