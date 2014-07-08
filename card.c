#include <stdio.h>
#include <stdarg.h>
#include "card.h"
#include "dir.h"

void error(char *s,...) {
  static char t[MAXSMALLS];
  va_list argptr;
  va_start(argptr,s);
  vsnprintf(t,MAXSMALLS-1,s,argptr);
  va_end(argptr);
	fprintf(stderr,t);
  exit(1);
}

/* one state per card */
struct card_t card[MAXCARDS];  
int cards;

void initcard() {
	/* load cards */
	dir_t dir;
	cards=0;
	if(!findfirst("cards/*",&dir)) error("couldn't read directory");
	do {
		/* TODO for each card, create lua context and register C functions */
		/* TODO process each file depending on file type.
		   txt files contain info
		   lua files contain script
		   possibly other files contain possibly other stuff
		*/
		card[cards].lua=luaL_newstate();
		printf("%s\n",dir.s);
		cards++;
	} while(findnext(&dir));
}

void shutdowncard() {
	int i;
	for(i=0;i<cards;i++) lua_close(card[i].lua);
}

int main() {
	initcard();
	
	shutdowncard();
	return 0;
}
