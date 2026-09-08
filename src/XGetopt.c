/*
 * getopt for Windows.
 *
 * The original XexTool compiled an XGetopt.c that is not present in the source
 * it was taken from; only the header survived. This is a fresh implementation
 * of the same interface, following POSIX getopt semantics.
 *
 * Options are single characters; a character followed by ':' in optstring
 * takes an argument, given either in the same argv element or the next one.
 * Returns the option character, '?' for an unrecognised option or a missing
 * argument, and -1 when the options are exhausted.
 */

#include <stdio.h>
#include <string.h>
#include "XGetopt.h"

int   optind = 1;    /* index of the next argv element to examine */
int   opterr = 1;    /* whether to report errors on stderr */
int   optopt = 0;    /* the option character that caused an error */
char *optarg = NULL; /* argument of the current option, if any */

static int s_charIndex = 1;  /* position within a clustered option group */

int getopt(int argc, char *argv[], char *optstring)
{
	int c;
	char *match;

	optarg = NULL;

	if (s_charIndex == 1)
	{
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return -1;

		if (strcmp(argv[optind], "--") == 0)
		{
			optind++;
			return -1;
		}
	}

	c = argv[optind][s_charIndex];
	optopt = c;
	match = (c == ':') ? NULL : strchr(optstring, c);

	if (match == NULL)
	{
		if (opterr)
			fprintf(stderr, "%s: unrecognised option -- %c\n", argv[0], c);
		/* step past this character, and past the group if it is exhausted */
		if (argv[optind][++s_charIndex] == '\0')
		{
			optind++;
			s_charIndex = 1;
		}
		return '?';
	}

	if (match[1] != ':')
	{
		/* no argument */
		if (argv[optind][++s_charIndex] == '\0')
		{
			optind++;
			s_charIndex = 1;
		}
		return c;
	}

	/* takes an argument: the rest of this element, or the next element */
	if (argv[optind][s_charIndex + 1] != '\0')
	{
		optarg = &argv[optind][s_charIndex + 1];
	}
	else if (++optind < argc)
	{
		optarg = argv[optind];
	}
	else
	{
		if (opterr)
			fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], c);
		s_charIndex = 1;
		return '?';
	}

	optind++;
	s_charIndex = 1;
	return c;
}
