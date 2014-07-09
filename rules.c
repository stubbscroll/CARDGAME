/* routines for dealing with game rules */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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

static char scans[MAXSMALLS];             /* scanf string limit */

static struct card_t card[MAXCARDS];      /* list of cards */
static int cards;
static struct player_t player[MAXPLAYER]; /* player info */
static int players;
static int currentplayer;

/* aux routines */

/* return 1 if s contains an integer, 0 otherwise */
static int isinteger(char *s) {
	if(*s=='-') s++;
	while(*s && isdigit(*s)) s++;
	return *s==0;
}

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

static void drawcard(int ix) {
	if(!player[ix].deckn) {
		if(!player[ix].discardn) return;
		discardtodeck(ix);
		shuffledeck(ix);
	}
	player[ix].hand[player[ix].handn++]=player[ix].deck[--player[ix].deckn];
}

/* player management ********************************************************/

static void resetplayer(int ix) {
	player[ix].deckn=0;
	player[ix].handn=0;
	player[ix].discardn=0;
	player[ix].playarean=0;
	player[ix].vp=0;
}

static void resetplayerturn(int ix) {
	/* TODO duration effects go here */
	player[ix].action=1;
	player[ix].money=0;
	player[ix].potion=0;
	player[ix].buy=1;
}

/* gain card into discard pile */
static void gaincard(int ix,int id) {
	player[ix].discard[player[ix].discardn++]=id;
}

/* play a card and activate its effects */
static void playcard(int id) {
	lua_getglobal(card[id].lua,"on_play");
	printf("about to call function on_play\n");
	if(lua_pcall(card[id].lua,0,0,0)) error("playcard: error calling 'on_play': %s\n",lua_tostring(card[id].lua,-1));
}

/* move card at from[ix] to to and make from compact */
static void movecard(int ix,int *from,int *lenf,int *to,int *lent) {
	to[(*lent)++]=from[ix];
	for(ix++;ix<*lenf;ix++) from[ix-1]=from[ix];
	(*lenf)--;
}

/* functions that are callable from lua scripts *****************************/

static int add_card(lua_State *L) {
	int argc=lua_gettop(L);
	if(argc!=1) error("add_card: expected 1 arg, got %d.\n",argc);
	return 0;
}

static int add_action(lua_State *L) {
	int argc=lua_gettop(L);
	if(argc!=1) error("add_card: expected 1 arg, got %d.\n",argc);
	return 0;
}

static int add_money(lua_State *L) {
	int argc;
	printf("in addmoney\n");
	argc=lua_gettop(L);
	if(argc!=1) error("add_card: expected 1 arg, got %d.\n",argc);
	/* add money! */
	player[currentplayer].money+=strtol(lua_tostring(L,0),0,10);
	return 0;
}

static int add_potion(lua_State *L) {
	int argc=lua_gettop(L);
	if(argc!=1) error("add_card: expected 1 arg, got %d.\n",argc);
	return 0;
}

static int add_buy(lua_State *L) {
	int argc=lua_gettop(L);
	if(argc!=1) error("add_card: expected 1 arg, got %d.\n",argc);
	return 0;
}

/* init etc *****************************************************************/

static void setcardproperty(int id,char *s) {
	if(!strcmp(s,"action")) card[id].type|=TYPE_ACTION;
	else if(!strcmp(s,"victory")) card[id].type|=TYPE_VICTORY;
	else if(!strcmp(s,"treasure")) card[id].type|=TYPE_TREASURE;
	else if(!strcmp(s,"attack")) card[id].type|=TYPE_ATTACK;
	else if(!strcmp(s,"curse")) card[id].type|=TYPE_CURSE;
	else if(!strcmp(s,"reaction")) card[id].type|=TYPE_REACTION;
	else if(!strcmp(s,"duration")) card[id].type|=TYPE_DURATION;
	else if(!strcmp(s,"looter")) card[id].type|=TYPE_LOOTER;
	else if(!strcmp(s,"ruins")) card[id].type|=TYPE_RUINS;
	else if(!strcmp(s,"shelter")) card[id].type|=TYPE_SHELTER;
	else error("setcardproperty: unknown type %s.\n",s);
}

static void parsecardtxt(int id,char *filename) {
	FILE *f;
	char s[MAXSMALLS],u[MAXSMALLS],c,v[MAXSMALLS];
	int i;
	strcpy(u,"cards/");
	strncat(u,filename,MAXSMALLS-1);
	if(!(f=fopen(u,"r"))) error("parsecardtxt: couldn't open file %s for reading.\n",filename);
	while(fscanf(f,scans,s)==1) {
		if(!strcmp(s,"type")) {
			if(fscanf(f,scans,v)!=1) error("parsecardtxt: unexpected end of file in %s reading property %s.\n",filename,s);
			if(strcmp(v,"{")) setcardproperty(id,v);
			else {
				while(1) {
					if(fscanf(f,scans,v)!=1) error("parsecardtxt: unexpected end of file in %s reading property %s.\n",filename,s);
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
				if(c=='\"') break;
				if(i<MAXSMALLS-1) card[id].fullname[i++]=c;
			} while(1);
			card[id].fullname[i]=0;
		} else error("parsecardtxt: unknown card property %s.\n",s);
	}
	if(fclose(f)) error("parsecardtxt: couldn't close file %s after reading.\n",filename);
}

void initcard() {
	dir_t dir;
	char s[MAXSMALLS];
	int id;
	/* create cached scanf snippet for reading string with length limit */
	snprintf(scans,MAXSMALLS,"%%%ds",MAXSMALLS);
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

/* everything below this point is extremely temporary placeholderish code to
   get very basic gameplay up */

struct player_t *cur;

/* return number of action cards on hand */
static int countcardsmask(int mask) {
	int r,i;
	for(r=i=0;i<player[currentplayer].handn;i++)
		if(card[player[currentplayer].hand[i]].type&mask) r++;
	return r;
}

static void actionphase() {
	int num,i,j;
	int list[MAXCARD];
	char s[MAXSMALLS];
	while((num=countcardsmask(TYPE_ACTION))) {
		printf("type 1 to %d to play an action card or 0 to skip.\n",num);
		for(i=j=0;i<cur->handn;i++)
			if(card[player[currentplayer].hand[i]].type&TYPE_ACTION) {
				list[j]=i;
				printf("  %d. %s\n",++j,card[player[currentplayer].hand[i]].shortname);
			}
		while(1) {
			scanf(scans,s);
			if(isinteger(s)) {
				j=strtol(s,0,10);
				if(!j) return;
				else {
					playcard(cur->hand[list[j-1]]);
					movecard(list[j-1],cur->hand,&cur->handn,cur->playarea,&cur->playarean);
					break;
				}
			}
		}
	}
}

static void buyphase() {
	int num,i,j;
	int list[MAXCARD];
	char s[MAXSMALLS];
	while((num=countcardsmask(TYPE_TREASURE))) {
		printf("type 1 to %d to play a treasure card or 0 to skip.\n",num);
		for(i=j=0;i<cur->handn;i++)
			if(card[player[currentplayer].hand[i]].type&TYPE_TREASURE) {
				list[j]=i;
				printf("  %d. %s\n",++j,card[player[currentplayer].hand[i]].shortname);
			}
		while(1) {
			scanf(scans,s);
			if(isinteger(s)) {
				j=strtol(s,0,10);
				if(!j) return;
				else {
					playcard(cur->hand[list[j-1]]);
					movecard(list[j-1],cur->hand,&cur->handn,cur->playarea,&cur->playarean);
					break;
				}
			}
		}
	}
}

static void playgame() {
	int i,j;
	for(i=0;i<players;i++) for(j=0;j<5;j++) drawcard(i);
	while(1) {
		cur=&player[currentplayer];
		printf("player %d plays:",currentplayer);
		resetplayerturn(currentplayer);
		for(i=0;i<cur->handn;i++) printf(" %s",card[cur->hand[i]].fullname);
		putchar('\n');
		actionphase();
		buyphase();
		break;

		/* draw 5 new cards */
		currentplayer=(currentplayer+1)%players;
	}
	puts("whee game over");
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
