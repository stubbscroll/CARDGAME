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

struct card_t {
	lua_State *lua;             /* lua script for card */
	char shortname[MAXSMALLS];  /* short card name */
	char fullname[MAXSMALLS];   /* full name with proper typesetting */
	int type;                   /* bitmask with card types */
	/* TODO detailed textual description for card */
	/* TODO card graphics */
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

struct piles_t {
	int pile[MAXCARD];
	int pilen;
	/* todo type. kingdom, supply, non-supply (such as prizes) */
};

void initcard();
void shutdowncard();

/* test */

void testplay();

/* TODO lua-interface functions like adding actions, cards, money, buys etc */

#endif /* CARD_H */
