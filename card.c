#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "card.h"

void error(char *s,...) {
	static char t[MAXSMALLS];
	va_list argptr;
	va_start(argptr,s);
	vsnprintf(t,MAXSMALLS-1,s,argptr);
	va_end(argptr);
	fprintf(stderr,t);
	exit(1);
}

int main() {
	initcard();
	testplay();
	shutdowncard();
	return 0;
}
