this is a card game which is suspiciously similar to another card game, and
the goal is to clone its functionality as well as adding new functionality and
customizability.

the main goal is to dabble in lua rather than obtaining a given end product.

gigantic warning: i can't lua. proceed at your own risk.
==============================================================================
SHORT-TERM TODO

- implement on_play() for the easy action cards
- implement add_cards_L() called by lua
- implement fancy cost function for peddler
- check for game over: empty province (or empty colony) or 3 empty supply
  piles
==============================================================================
LONG-TERM TODO

- make a proper interface in rules.c that calls player agents (ui or ai)
- finish the game
==============================================================================
open problems

- none!
==============================================================================
pitfalls that i'm probably not going to fix

- in the unfortunate case that all kingdom cards with cost 2 or 3 are in the
  supply and young witch is in the game, the setup for young witch will crash
  since it can't find a bane card. the lazy fix is to make sure there is no
  file setup with fewer than 10 kingdom cards with cost 2 or 3, and the "ugly"
  fix is to force a reshuffle or something.
==============================================================================
actual documentation

this game contains individual cards, and they are arranged in groups. some
groups contain different cards, such as ruins and knights. most groups have
one unique card. each card is defined in two files, [card].txt and
[card].lua. the .txt file contains static information, and the .lua file
contains scripting code in lua.
------------------------------------------------------------------------------
.txt file format:

type a
type { a b ... }

the type of the card. can be at least 1 out of: action victory treasure
attack curse, reaction, duration, looter, ruins, shelter, prize.

name "string"

the full name of the card.

kingdom

the card can be chosen as a kingdom card.
------------------------------------------------------------------------------
the .lua file must contain the following functions:

on_setup()

sets up a pile for the card (supply or otherwise)

post_setup()

post-processing of setup for this specific card, needed in special cases such
as young witch, black market, urchin, tournament etc.

on_play()

this is called when a card is played in the action or buy phases.

can_buy()

returns whether it's possible to buy the card in the given situation.
example: can't buy grand market with copper in play.

victory_points()

returns the number of victory points the card is worth, which can be
non-constant for cards such as garden.

money_cost()

returns the cost of the card in money. can be non-constant for cards like
peddler.

potion_cost()

returns the cost of the card in potions. to my knowledge, no cards so far
have non-constant potion cost.

more to be added.
==============================================================================
examples of interesting scripts in cards

not many so far, but:
- youngwitch, post_setup()
- grand market, can_buy()
==============================================================================
