/* routines for dealing with game rules */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "card.h"
#include "dir.h"

/* stuff to support
   - buying cards such as grand market which can not be bought with copper in
     play
   - reaction cards, duration cards etc. in particular, moat that blocks all
     attacks
   - victory cards with scripted number of victory points, like garden
   - treasure cards with scripted number of money, like bank and fool's gold
   - trade route, with trade route mat and token on each victory card supply
   - victory point tokens
   - young witch and bane cards
   - distinguish with in-play effects and added effects
   - "while in play" effects, such as hoard (when you buy a victory card, gain
     a gold)
   - peddler! the discount should not be applied when outside buy phase (for
     instance, when using swindler, jester etc)
   - black market and its special rules
   - possession and all its special rules
   suggestions
   - have can_buy() in each card returning true/false
   - have get_money() in each treasure card returning value
   - have get_victory() in each victory card returning value
   todo
   - learn to call lua functions from c!
*/

static struct card_t card[MAXCARDS];
static int cards;
static struct player_t player[MAXPLAYER];
static int players;
static int currentplayer;

/* card management **********************************************************/

/* given short card name, return index into card list */
static int getcardid(char *s) {
	int i;
	for(i=0;i<cards;i++) if(!strcmp(s,card[i].shortname)) return i;
	return -1;
}

/* move discard to deck */
static void discardtodeck(int ix) {
	int i;
	if(player[ix].deckn) error("discardtodraw: deck isn't empty.\n");
	for(i=0;i<player[ix].discardn;i++) player[ix].deck[i]=player[ix].discard[i];
	player[ix].deckn=player[ix].discardn;
	player[ix].discardn=0;
}

/* shuffle deck, uses modern fisher-yates shuffle */
static void shuffledeck(int ix) {
	int i,j,t;
	for(i=player[ix].deckn-1;i;i--) {
		j=rand()%(i+1);
		t=player[ix].deck[j];
		player[ix].deck[j]=player[ix].deck[i];
		player[ix].deck[i]=t;
	}
}

/* player management ********************************************************/

static void resetplayer(int ix) {
	player[ix].deckn=0;
	player[ix].handn=0;
	player[ix].discardn=0;
	player[ix].vp=0;
}

/* gain card into discard pile */
static void gaincard(int ix,int id) {
	player[ix].discard[player[ix].discardn++]=id;
}

/* functions that are callable from lua scripts *****************************/

static int add_card(lua_State *L) { return 0; }
static int add_action(lua_State *L) { return 0; }
static int add_money(lua_State *L) { return 0; }
static int add_potion(lua_State *L) { return 0; }
static int add_buy(lua_State *L) { return 0; }

/* init etc *****************************************************************/

#define TYPE_ACTION 1
#define TYPE_VICTORY 2
#define TYPE_TREASURE 4
#define TYPE_ATTACK 8
#define TYPE_CURSE 16
#define TYPE_REACTION 32
#define TYPE_DURATION 64
#define TYPE_LOOTER 128

static void setcardproperty(int id,char *s) {
	if(!strcmp(s,"action")) card[id].type|=TYPE_ACTION;
	else if(!strcmp(s,"victory")) card[id].type|=TYPE_VICTORY;
	else if(!strcmp(s,"treasure")) card[id].type|=TYPE_TREASURE;
	else if(!strcmp(s,"attack")) card[id].type|=TYPE_ATTACK;
	else if(!strcmp(s,"curse")) card[id].type|=TYPE_CURSE;
	else if(!strcmp(s,"reaction")) card[id].type|=TYPE_REACTION;
	else if(!strcmp(s,"duration")) card[id].type|=TYPE_DURATION;
	else if(!strcmp(s,"looter")) card[id].type|=TYPE_LOOTER;
}

static void parsecardtxt(int id,char *filename) {
	FILE *f;
	char s[MAXSMALLS],t[MAXSMALLS],u[MAXSMALLS],c,v[MAXSMALLS];
	int i;
	strcpy(u,"cards/");
	strncat(u,filename,MAXSMALLS-1);
	if(!(f=fopen(u,"r"))) error("parsecardtxt: couldn't open file %s for reading.\n",filename);
	snprintf(t,MAXSMALLS,"%%%ds",MAXSMALLS);
	while(fscanf(f,t,s)==1) {
		if(!strcmp(s,"type")) {
			if(fscanf(f,t,v)!=1) error("parsecardtxt: unexpected end of file in %s reading property %s.\n",filename,s);
			if(strcmp(v,"{")) setcardproperty(id,v);
			else {
				while(1) {
					if(fscanf(f,t,v)!=1) error("parsecardtxt: unexpected end of file in %s reading property %s.\n",filename,s);
					if(!strcmp(v,"}")) break;
					setcardproperty(id,v);
				}
			}
		} else if(!strcmp(s,"name")) {
			do {
				if((c=fgetc(f))==EOF) error("parsecardtxt: expected \" in file %s when reading property %s.\n",filename,s);
			} while(c!='\"');
			i=0;
			do {
				if((c=fgetc(f))==EOF) error("parsecardtxt: expected \" in file %s when reading property %s.\n",filename,s);
				if(i<MAXSMALLS-1) card[id].fullname[i++]=c;
			} while(c!='\"');
			card[id].fullname[i]=0;
		} else error("parsecardtxt: unknown card property %s.\n",s);
	}
	if(fclose(f)) error("parsecardtxt: couldn't close file %s after reading.\n",filename);
}

void initcard() {
	dir_t dir;
	char s[MAXSMALLS];
	int id;
	/* load cards */
	cards=0;
	if(!findfirst("cards/*",&dir)) error("couldn't read directory");
	do {
		getbasefilename(dir.s,s,MAXSMALLS);
		id=getcardid(s);
		if(id<0) {
			id=cards++;
			strncpy(card[id].shortname,s,MAXSMALLS);
			card[id].type=0;
			/* lua stuff */
			card[id].lua=luaL_newstate();
			luaL_openlibs(card[id].lua);
			if(!luaL_loadfile(card[id].lua,s)) error("initcard: lua error [%s]\n",lua_tostring(card[id].lua,-1));
			lua_register(card[id].lua,"add_card",add_card);
			lua_register(card[id].lua,"add_action",add_action);
			lua_register(card[id].lua,"add_money",add_money);
			lua_register(card[id].lua,"add_buy",add_buy);
			lua_register(card[id].lua,"add_potion",add_potion);
		}
		getextension(dir.s,s,MAXSMALLS);
		/* process files by extension, some contain info */
		if(!strcmp(s,"txt")) {
			/* read card type and long description */
			parsecardtxt(id,dir.s);
		}
		/* TODO add other file types later such as images */
	} while(findnext(&dir));
}

void shutdowncard() {
	int i;
	for(i=0;i<cards;i++) lua_close(card[i].lua);
}

/* test */

static void playgame() {
	printf("whee, entering infinite play loop\n");
	while(1) {
		
		
		
		currentplayer=(currentplayer+1)%players;
	}
}

void testplay() {
	int i,j,c;
	players=2;
	for(i=0;i<players;i++) {
		resetplayer(i);
		c=getcardid("copper");
		if(c<0) error("testplay: couldn't find copper.\n");
		for(j=0;j<7;j++) gaincard(i,c);
		c=getcardid("estate");
		if(c<0) error("testplay: couldn't find estate.\n");
		for(j=0;j<3;j++) gaincard(i,c);
	}
	currentplayer=0;
	playgame();
}
