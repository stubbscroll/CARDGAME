#ifndef CARD_H
#define CARD_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* game.c */
#define MAXCARDS 1000
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

struct card_t {
	lua_State *lua;             /* lua script for card */
	char name[MAXSMALLS];       /* short card name */
	int type;                   /* bitmask with card types */
	int money;                  /* cost of card in money */
	int potion;                 /* cost of card in potions */
	/* TODO detailed textual description for card */
	/* TODO card graphics */
	/* TODO pointer-thing to lua function */
};

extern struct card_t card[MAXCARDS]; /* cards */
extern int cards;                    /* number of cards */

/* rules.c */

/* TODO lua-interface functions like adding actions, cards, money, buys etc */

#endif /* CARD_H */
