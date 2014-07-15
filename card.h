#ifndef CARD_H
#define CARD_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* game.c */

void error(char *,...);

/* rules.c */

#define MAXSMALLS 127
#define MAXLARGES 65535

/* card type masks */
#define TYPE_ACTION 1
#define TYPE_VICTORY 2
#define TYPE_TREASURE 4
#define TYPE_ATTACK 8
#define TYPE_CURSE 16
#define TYPE_REACTION 32
#define TYPE_DURATION 64
#define TYPE_LOOTER 128
#define TYPE_RUINS 256
#define TYPE_SHELTER 512
#define TYPE_PRIZE 1024

#define MAXCARDS 1000

/* one entry for each different card */
struct card_t {
	lua_State *lua;             /* lua script for card */
	char shortname[MAXSMALLS];  /* short card name */
	char fullname[MAXSMALLS];   /* full name with proper typesetting */
	int type;                   /* bitmask with card types */
	int groupid;                /* pointer to group[] */
	char iskingdom;							/* 1 if cards in group can be a kingdom pile */
	/* TODO detailed textual description for card */
	/* TODO card graphics */
};

/* groups! a group can contain different cards, like ruins or knights */
struct group_t {
	char name[MAXSMALLS];
	int card[MAXCARDS];         /* id of each card in group */
	int cards;                  /* number of cards in group */
	char taken;                 /* 1 if group is in the game */
	char iskingdom;							/* 1 if cards in group can be a kingdom pile */
	/* TODO which set the group belongs to. used for deciding whether to use
	   colony/platinum and ruins */
};

#define MAXCARD 300
#define MAXPLAYER 10
#define MAXPILES 25

struct player_t {
	int deck[MAXCARD];      /* cards in deck (index into card[]) */
	int deckn;              /* number of cards in deck */
	int hand[MAXCARD];      /* cards in hand */
	int handn;
	int discard[MAXCARD];   /* discard pile */
	int discardn;
	int playarea[MAXCARD];  /* play area */
	int playarean;

	int vp;                 /* number of victory point tokens */

	int action;             /* number of actions left */
	int money;              /* amount of money to spend */   
	int potion;             /* amount of potions to spend */
	int buy;                /* number of buys */
};

struct pile_t {
	int card[MAXCARD];      /* the cards in this pile, highest index is top */
	int cards;              /* number of cards in this pile */
	/* todo type:
	   kingdom
	   bane
	   black market
	   supply or not (such as prizes, spoils, madman, mercenary and so on)
	*/
};

void initcard();
void shutdowncard();

/* test */

void testplay();

/* TODO lua-interface functions like adding actions, cards, money, buys etc */

#endif /* CARD_H */
