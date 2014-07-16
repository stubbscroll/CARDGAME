/* routines for dealing with game rules */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
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
   - black market and its special rules (some examples below)
   - possession and all its special rules
   - piles with different cards: ruins and knights. as it stands now, each
     different card would need its own file, but should somehow be grouped
   - strange trigging sequences, such as young witch draws black market as
     bane card which triggers black market setup
   - players buys young witch via black market which triggers bane setup
   suggestions
   - have can_buy() in each card returning true/false
   - have get_money() in each treasure card returning value
   - have get_victory() in each victory card returning value

   - the grand dilemma: have many convenient c functions to do stuff, or
     merely let lua have access to all variables and do as much as possible in
     lua?
   - which variables should lua get access to? where to draw the border?
*/

static char scans[MAXSMALLS];             /* scanf string limit snippet */

static struct card_t card[MAXCARDS];      /* list of cards */
static int cards;
static struct group_t group[MAXCARDS];    /* list of groups (ruins etc) */
static int groups;
static struct pile_t pile[MAXPILES];      /* supply etc */
static int piles;
static struct player_t player[MAXPLAYER]; /* player info */
static int players;
static int currentplayer;

/* debugprint */

static void dumpcards() {
	int i;
	printf("%d cards\n",cards);
	for(i=0;i<cards;i++) {
		printf("[%s \"%s\"",card[i].shortname,card[i].fullname);
		if(card[i].type&TYPE_ACTION) printf(" ACTION");
		if(card[i].type&TYPE_VICTORY) printf(" VICTORY");
		if(card[i].type&TYPE_TREASURE) printf(" TREASURE");
		if(card[i].type&TYPE_ATTACK) printf(" ATTACK");
		if(card[i].type&TYPE_CURSE) printf(" CURSE");
		if(card[i].type&TYPE_REACTION) printf(" REACTION");
		if(card[i].type&TYPE_DURATION) printf(" DURATION");
		if(card[i].type&TYPE_LOOTER) printf(" LOOTER");
		if(card[i].type&TYPE_RUINS) printf(" RUINS");
		if(card[i].type&TYPE_SHELTER) printf(" SHELTER");
		if(card[i].type&TYPE_PRIZE) printf(" PRIZE");
		printf(" %s]\n",group[card[i].groupid].name);
	}
	puts("===============================================================================");
}

static void dumpgroups() {
	int i,j;
	printf("%d groups\n",groups);
	for(i=0;i<groups;i++) {
		printf("[%s %d:",group[i].name,group[i].iskingdom);
		for(j=0;j<group[i].cards;j++) printf(" %s",card[group[i].card[j]].shortname);
		printf("]\n");
	}
	puts("===============================================================================");
}

static void dumppiles() {
	int i,j;
	printf("%d piles\n",piles);
	for(i=0;i<piles;i++) {
		printf("[%d",pile[i].cards);
		if(pile[i].flag[0]&1) printf(" (bane)");
		for(j=0;j<pile[i].cards;j++) printf(" %s",card[pile[i].card[j]].shortname);
		printf("]\n");
	}
	puts("===============================================================================");
}

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

/* group (of cards) management */

/* given group name, return index into group list */
static int getgroupid(char *s) {
	int i;
	for(i=0;i<groups;i++) if(!strcmp(s,group[i].name)) return i;
	return -1;
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
	if(lua_pcall(card[id].lua,0,0,0)) error("playcard: error calling 'on_play': %s\n",lua_tostring(card[id].lua,-1));
}

/* move card at from[ix] to to and make from compact */
static void movecard(int ix,int *from,int *lenf,int *to,int *lent) {
	to[(*lent)++]=from[ix];
	for(ix++;ix<*lenf;ix++) from[ix-1]=from[ix];
	(*lenf)--;
}

/* move all cards from pile from to pile to */
static void movepile(int *from,int *lenf,int *to,int *lent) {
	/* todo */
}

/* pile management **********************************************************/

static void initpile(int pid) {
	int i;
	pile[pid].cards=0;
	pile[pid].supply=0;
	for(i=0;i<(MAXFLAG+31)/32;i++) pile[pid].flag[i]=0;
	for(i=0;i<MAXVAR;i++) pile[pid].var[i]=0;
}

/* new game setup management ************************************************/

/* add pile by group id! */
static void addpiletogame(int gid) {
	int id=group[gid].card[0];
	lua_getglobal(card[id].lua,"on_setup");
	if(lua_pcall(card[id].lua,0,0,0)) error("addpiletogame: %s\n",lua_tostring(card[id].lua,-1));
}

static void addpiletogamebyname(char *s) {
	int id=getgroupid(s);
	if(id<0) error("generatenewgame: no %s.\n",s);
	addpiletogame(id);
}

#define KINGDOM 10
/* picks 10 kingdom cards and deals out cards to all players */
static void generatenewgame() {
	int i,j,id;
	for(i=0;i<groups;i++) group[i].taken=0;
	piles=0;
	/* always add copper, silver, gold, estate, duchy, province, curse */
	addpiletogamebyname("copper");
	addpiletogamebyname("silver");
	addpiletogamebyname("gold");
	addpiletogamebyname("estate");
	addpiletogamebyname("duchy");
	addpiletogamebyname("province");
	addpiletogamebyname("curse");
	for(i=0;i<KINGDOM;i++) {
		do j=rand()%groups; while(group[j].taken || !group[j].iskingdom);
		/* TODO add pile to game */
		printf("kingdom card %d: %s\n",j+1,group[j].name);
		group[j].taken=1;
		addpiletogame(j);
	}
	/* post-process setup! for special cases like young witch */
	for(i=0;i<piles;i++) {
		if(!pile[i].cards) error("generatenewgame: found empty pile.\n");
		id=pile[i].card[0];
		if(card[id].iskingdom) {
			lua_getglobal(card[id].lua,"post_setup");
			if(lua_pcall(card[id].lua,0,0,0)) error("generatenewgame: %s\n",lua_tostring(card[id].lua,-1));
		}
	}
}
#undef KINGDOM

/* functions that call lua scripts ******************************************/

static int victory_points_L(int id) {
	/* TODO */
	return 0;
}

static int money_cost_L(int id) {
	int v;
	lua_getglobal(card[id].lua,"money_cost");
	if(lua_pcall(card[id].lua,0,1,0)) error("get_card_money_cost: %s\n",lua_tostring(card[id].lua,-1));
	if(!lua_isnumber(card[id].lua,-1)) error("get_card_money_cost: expected number, got %s.\n",lua_tostring(card[id].lua,-1));
	v=lua_tonumber(card[id].lua,-1);
	lua_pop(card[id].lua,1);
	return v;
}

static int potion_cost_L(int id) {
	int v;
	lua_getglobal(card[id].lua,"potion_cost");
	if(lua_pcall(card[id].lua,0,1,0)) error("get_card_money_cost: %s\n",lua_tostring(card[id].lua,-1));
	if(!lua_isnumber(card[id].lua,-1)) error("get_card_money_cost: expected number, got %s.\n",lua_tostring(card[id].lua,-1));
	v=lua_tonumber(card[id].lua,-1);
	lua_pop(card[id].lua,1);
	return v;
}


/* functions that are callable from lua scripts *****************************/
/* all these functions are prefixed with L_, and they are called in lua scripts
   with the identifier without prefix */

/* given index, return cost of card in money */
static int L_get_card_money_cost(lua_State *L) {
	int id;
	if(lua_gettop(L)!=1) error("get_card_money_cost: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_card_money_cost: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	id=strtol(lua_tostring(L,1),0,10);
	if(id<0 || id>=cards) error("get_card_money_cost: illegal index %d.\n",id);
	lua_pushnumber(L,money_cost_L(id));
	return 1;
}

/* given index, return cost of card in potions */
static int L_get_card_potion_cost(lua_State *L) {
	int id;
	if(lua_gettop(L)!=1) error("get_card_potion_cost: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_card_potion_cost: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	id=strtol(lua_tostring(L,1),0,10);
	if(id<0 || id>=cards) error("get_card_potion_cost: illegal index %d.\n",id);
	lua_pushnumber(L,potion_cost_L(id));
	return 1;
}

/* get card[i].groupid */
static int L_get_card_groupid(lua_State *L) {
	int i;
	if(lua_gettop(L)!=1) error("get_card_groupid: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_card_groupid: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	i=strtol(lua_tostring(L,1),0,10);
	if(i<0 || i>=cards) error("get_card_groupid: found %d, should be between 0 and %d.\n",i,cards-1);
	lua_pushnumber(L,card[i].groupid);
	return 1;
}

/* set group[i].taken */
/* TODO not used so far */
static int L_set_group_taken(lua_State *L) {
	int i,j;
	if(lua_gettop(L)!=2) error("set_group_taken: expected 2 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("set_group_taken: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("set_group_taken: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	if(i<0 || i>=groups) error("set_group_taken: illegal group number %d.\n",i);
	if(j<0 || j>1) error("set_group_taken: value must be 0 or 1, found %d.\n",j);
	group[i].taken=j;
	return 0;
}

/* set all fields in pile to zero */
static int L_init_pile(lua_State *L) {
	int i;
	if(lua_gettop(L)!=1) error("init_pile: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("init_pile: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	i=strtol(lua_tostring(L,1),0,10);
	if(i<0 || i>=piles) error("init_pile: index %d out of bounds.\n",i);
	initpile(i);
	return 0;
}

/* get pile[i].card[j] */
static int L_get_pile_card(lua_State *L) {
	int i,j;
	if(lua_gettop(L)!=2) error("get_pile_cards: expected 2 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_pile_cards: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("get_pile_cards: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	if(i<0 || i>=piles) error("get_pile_cards: illegal pile number %d.\n",i);
	if(j<0 || j>=pile[i].cards) error("get_pile_cards: illegal card number %d in pile %d.\n",j,i);
	lua_pushnumber(L,pile[i].card[j]);
	return 1;
}

/* set pile[i].card[j] */
static int L_set_pile_card(lua_State *L) {
	int i,j,k;
	if(lua_gettop(L)!=3) error("set_pile_cards: expected 3 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("set_pile_cards: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("set_pile_cards: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	if(!lua_isnumber(L,3)) error("set_pile_cards: arg 3 must be int, found %s.\n",lua_tostring(L,3));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	k=strtol(lua_tostring(L,3),0,10);
	if(i<0 || i>=piles) error("set_pile_cards: illegal pile number %d.\n",i);
	if(j<0 || j>=pile[i].cards) error("set_pile_cards: illegal card number %d in pile %d.\n",j,i);
	if(k<0 || k>=cards) error("set_pile_cards: illegal card id %d.\n",k);
	pile[i].card[j]=k;
	return 0;
}

/* set pile[i].cards */
static int L_set_pile_cards(lua_State *L) {
	int i,j;
	if(lua_gettop(L)!=2) error("set_pile_cards: expected 2 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("set_pile_cards: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("set_pile_cards: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	if(i<0 || i>=piles) error("set_pile_cards: illegal pile number %d.\n",i);
	if(j<0) error("set_pile_cards: cards must be nonnegative, found %d.\n",j);
	pile[i].cards=j;
	return 0;
}

/* set pile[i].flsg[j] */
static int L_set_pile_flag(lua_State *L) {
	int i,j,k,mask;
	if(lua_gettop(L)!=3) error("set_pile_bane: expected 3 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("set_pile_bane: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("set_pile_bane: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	if(!lua_isnumber(L,3)) error("set_pile_bane: arg 3 must be int, found %s.\n",lua_tostring(L,2));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	k=strtol(lua_tostring(L,3),0,10);
	if(i<0 || i>=piles) error("set_pile_bane: illegal pile number %d.\n",i);
	if(j<0 || j>MAXFLAG) error("set_pile_bane: second arg out of bounds.\n",j);
	if(k<0 || j>k) error("set_pile_bane: third arg must be 0 or 1\n",j);
	mask=((uint32_t)-1)-(((uint32_t)1)<<(j&31));
	pile[i].flag[j]=(pile[i].flag[j]&mask)|(((uint32_t)1)<<(j&31));
	return 0;
}

/* set pile[i].supply */
static int L_set_pile_supply(lua_State *L) {
	int i,j;
	if(lua_gettop(L)!=2) error("set_pile_supply: expected 2 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("set_pile_supply: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("set_pile_supply: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	if(i<0 || i>=piles) error("set_pile_supply: illegal pile number %d.\n",i);
	if(j<0 || j>1) error("set_pile_supply: second arg must be 0 or 1\n",j);
	pile[i].supply=j;
	return 0;
}

/* set piles */
static int L_set_piles(lua_State *L) {
	int i;
	if(lua_gettop(L)!=1) error("get_piles: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("set_piles: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	i=strtol(lua_tostring(L,1),0,10);
	if(i<0) error("set_piles: found %d, should be nonnegative.\n",i);
	piles=i;
	return 0;
}

/* get piles */
static int L_get_piles(lua_State *L) {
	if(lua_gettop(L)!=0) error("get_piles: expected 0 args, got %d.\n",lua_gettop(L));
	lua_pushnumber(L,piles);
	return 1;
}

/* get group[i].card[j] */
static int L_get_group_card(lua_State *L) {
	int i,j;
	if(lua_gettop(L)!=2) error("get_group_card: expected 2 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_group_card: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	if(!lua_isnumber(L,2)) error("get_group_card: arg 2 must be int, found %s.\n",lua_tostring(L,2));
	i=strtol(lua_tostring(L,1),0,10);
	j=strtol(lua_tostring(L,2),0,10);
	if(i<0 || i>=groups) error("get_group_card: illegal pile number %d.\n",i);
	if(j<0 || j>=group[i].cards) error("get_group_card: index out of range.\n",j);
	lua_pushnumber(L,group[i].card[j]);
	return 1;
}

/* get group[i].taken */
static int L_get_group_taken(lua_State *L) {
	int i;
	if(lua_gettop(L)!=1) error("get_group_taken: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_group_taken: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	i=strtol(lua_tostring(L,1),0,10);
	if(i<0 || i>=groups) error("get_group_taken: found %d, should be between 0 and %d.\n",i,groups-1);
	lua_pushnumber(L,group[i].taken);
	return 1;
}

/* get group[i].iskingdom */
static int L_get_group_iskingdom(lua_State *L) {
	int i;
	if(lua_gettop(L)!=1) error("get_group_iskingdom: expected 1 args, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("get_group_iskingdom: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	i=strtol(lua_tostring(L,1),0,10);
	if(i<0 || i>=groups) error("get_group_iskingdom: found %d, should be between 0 and %d.\n",i,groups-1);
	lua_pushnumber(L,group[i].iskingdom);
	return 1;
}

/* get piles */
static int L_get_groups(lua_State *L) {
	if(lua_gettop(L)!=0) error("get_groups: expected 0 args, got %d.\n",lua_gettop(L));
	lua_pushnumber(L,groups);
	return 1;
}

/* get players */
static int L_get_players(lua_State *L) {
	if(lua_gettop(L)!=0) error("get_players: expected 0 args, got %d.\n",lua_gettop(L));
	lua_pushnumber(L,players);
	return 1;
}

/* give current player cards */
static int L_add_card(lua_State *L) {
	if(lua_gettop(L)!=1) error("add_card: expected 1 arg, got %d.\n",lua_gettop(L));
	/* todo draw cards */
	return 0;
}

/* give current player more actions */
static int L_add_action(lua_State *L) {
	if(lua_gettop(L)!=1) error("add_card: expected 1 arg, got %d.\n",lua_gettop(L));
	/* give more actions to current player */
	player[currentplayer].action+=strtol(lua_tostring(L,1),0,10);
	return 0;
}

/* give current player more money */
static int L_add_money(lua_State *L) {
	if(lua_gettop(L)!=1) error("add_card: expected 1 arg, got %d.\n",lua_gettop(L));
	/* give more money to current player */
	player[currentplayer].money+=strtol(lua_tostring(L,1),0,10);
	return 0;
}

/* give current player more potions */
static int L_add_potion(lua_State *L) {
	if(lua_gettop(L)!=1) error("add_card: expected 1 arg, got %d.\n",lua_gettop(L));
	/* give more potion to current player */
	player[currentplayer].potion+=strtol(lua_tostring(L,1),0,10);
	return 0;
}

/* give current player more buys */
static int L_add_buy(lua_State *L) {
	if(lua_gettop(L)!=1) error("add_card: expected 1 arg, got %d.\n",lua_gettop(L));
	/* give more buys to current player */
	player[currentplayer].buy+=strtol(lua_tostring(L,1),0,10);
	return 0;
}

/* add pile to game (groupid) */
static int L_add_pile(lua_State *L) {
	int i;
	if(lua_gettop(L)!=1) error("add_pile: expected 1 arg, got %d.\n",lua_gettop(L));
	if(!lua_isnumber(L,1)) error("add_pile: arg 1 must be int, found %s.\n",lua_tostring(L,1));
	i=strtol(lua_tostring(L,1),0,10);
	addpiletogame(i);
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
	else if(!strcmp(s,"prize")) card[id].type|=TYPE_PRIZE;
	else error("setcardproperty: unknown type %s.\n",s);
}

/* special routine for finding group id, and creating it if it doesn't exist */
static int getgroupidcreate(char *s) {
	int i;
	for(i=0;i<groups;i++) if(!strcmp(s,group[i].name)) return i;
	strncpy(group[groups].name,s,MAXSMALLS-1);
	group[groups].cards=0;
	return groups++;
}

static void parsecardtxt(int id,char *filename) {
	FILE *f;
	char s[MAXSMALLS],u[MAXSMALLS],c,v[MAXSMALLS];
	int i,gid,foundgroup=0;
	strncpy(u,"cards/",MAXSMALLS-1);
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
		} else if(!strcmp(s,"group")) {
			if(fscanf(f,scans,v)!=1) error("parsecardtxt: unexpected end of file in %s reading property %s.\n",filename,s);
			gid=getgroupidcreate(v);
			card[id].groupid=gid;
			group[gid].card[group[gid].cards++]=id;
			foundgroup=1;
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
		} else if(!strcmp(s,"kingdom")) {
			card[id].iskingdom=1;
		} else error("parsecardtxt: unknown card property %s.\n",s);
	}
	if(fclose(f)) error("parsecardtxt: couldn't close file %s after reading.\n",filename);
	/* no group property found: set equal to filename or something */
	if(!foundgroup) {
		gid=getgroupidcreate(filename);
		card[id].groupid=gid;
		group[gid].card[group[gid].cards++]=id;
	}
}

void initcard() {
	dir_t dir;
	char s[MAXSMALLS],t[MAXSMALLS];
	int id,i,j,mask;
	/* create cached scanf snippet for reading string with length limit */
	snprintf(scans,MAXSMALLS,"%%%ds",MAXSMALLS);
	/* load cards */
	cards=groups=0;
	if(!findfirst("cards/*",&dir)) error("couldn't read directory");
	do {
		if(!strcmp(dir.s,".") || !strcmp(dir.s,"..")) continue;
		getbasefilename(dir.s,s,MAXSMALLS);
		id=getcardid(s);
		if(id<0) {
			id=cards++;
			strncpy(card[id].shortname,s,MAXSMALLS);
			card[id].type=0;
			card[id].iskingdom=0;
			/* lua stuff */
			if(!(card[id].lua=luaL_newstate())) error("initcard: luaL_newstate failed.\n");
			luaL_openlibs(card[id].lua);
			lua_register(card[id].lua,"get_card_money_cost",L_get_card_money_cost);
			lua_register(card[id].lua,"get_card_potion_cost",L_get_card_potion_cost);
			lua_register(card[id].lua,"get_card_groupid",L_get_card_groupid);
			lua_register(card[id].lua,"init_pile",L_init_pile);
			lua_register(card[id].lua,"get_pile_card",L_get_pile_card);
			lua_register(card[id].lua,"set_pile_card",L_set_pile_card);
			lua_register(card[id].lua,"set_pile_cards",L_set_pile_cards);
			lua_register(card[id].lua,"set_pile_flag",L_set_pile_flag);
			lua_register(card[id].lua,"set_pile_supply",L_set_pile_supply);
			lua_register(card[id].lua,"set_piles",L_set_piles);
			lua_register(card[id].lua,"get_piles",L_get_piles);
			lua_register(card[id].lua,"get_group_card",L_get_group_card);
			lua_register(card[id].lua,"get_group_taken",L_get_group_taken);
			lua_register(card[id].lua,"get_group_iskingdom",L_get_group_iskingdom);
			lua_register(card[id].lua,"get_groups",L_get_groups);
			lua_register(card[id].lua,"get_players",L_get_players);
			lua_register(card[id].lua,"add_card",L_add_card);
			lua_register(card[id].lua,"add_action",L_add_action);
			lua_register(card[id].lua,"add_money",L_add_money);
			lua_register(card[id].lua,"add_buy",L_add_buy);
			lua_register(card[id].lua,"add_potion",L_add_potion);
			lua_register(card[id].lua,"add_pile",L_add_pile);
		}
		getextension(dir.s,s,MAXSMALLS);
		/* process files by extension, some contain info */
		if(!strcmp(s,"txt")) {
			/* read card type and long description */
			parsecardtxt(id,dir.s);
		} else if(!strcmp(s,"lua")) {
			/* load script */
			strncpy(t,"cards/",MAXSMALLS-1);
			strncat(t,dir.s,MAXSMALLS-1);
			if(luaL_loadfile(card[id].lua,t)) error("initcard: lua error [%s]\n",lua_tostring(card[id].lua,-1));
			/* priming run of script! crucial for avoiding mysterious crash! */
			if(lua_pcall(card[id].lua,0,0,0)) error("initcard: lua_pcall failed: %s\n",lua_tostring(card[id].lua,-1));
		}
		/* TODO add other file types later such as images */
	} while(findnext(&dir));
	/* for each card, set thiscardid and thisgroupid */
	for(i=0;i<cards;i++) {
		lua_pushnumber(card[i].lua,i);
		lua_setglobal(card[i].lua,"thiscardid");
		lua_pushnumber(card[i].lua,card[i].groupid);
		lua_setglobal(card[i].lua,"thisgroupid");
	}
	/* update group.iskingdom */
	for(i=0;i<groups;i++) {
		for(mask=j=0;j<cards;j++) if(card[j].groupid==i) mask|=1<<card[j].iskingdom;
		if(mask==3) error("initcard: group %s has both kingdom and non-kingdom cards.\n",group[i].name);
		group[i].iskingdom=mask>1;
	}
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
				printf("  %d. %s\n",++j,card[player[currentplayer].hand[i]].fullname);
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
				printf("  %d. %s\n",++j,card[player[currentplayer].hand[i]].fullname);
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
		printf("player %d plays and has",currentplayer);
		resetplayerturn(currentplayer);
		for(i=0;i<cur->handn;i++) printf(" %s",card[cur->hand[i]].fullname);
		putchar('\n');
		puts("action phase!");
		actionphase();
		puts("buy phase!");
		buyphase();

		printf("player has %d actions, %d money, %d buys, %d potions\n",cur->action,cur->money,cur->buy,cur->potion);
		break;

		/* draw 5 new cards */
		currentplayer=(currentplayer+1)%players;
	}
	dumpcards();
	dumpgroups();
	dumppiles();
	puts("whee game over");
}

void testplay() {
	int i,j,c;
	players=2;
	srand(time(0));
	generatenewgame();
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
