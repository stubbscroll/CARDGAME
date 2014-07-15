this is a card game which is suspiciously similar to another card game, and
the goal is to clone its functionality as well as adding new functionality and
customizability.

the main goal is to dabble in lua rather than obtaining a given end product.

gigantic warning: i can't lua. proceed at your own risk.
==============================================================================
TODO

- for all cards, add set_pile_bane(pileid,0) somewhere in on_setup().
- for all supply cards, add set_pile_supply(pileid,1) somewhere in on_setup().
  for non-supply cards, add set_pile_supply(pileid,0).
==============================================================================
open problems
- the program crashes when i try to call a non-existing lua function. desired
  behaviour: return with error message
- to add to the above: lua crashes pretty much any time an error occurs.
  consider following the steps here
  http://stackoverflow.com/questions/19903762/lua-5-2-compiled-with-mingw32-crashes-my-host-program-when-a-lua-error-occurs
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

this happens when a card is played in the action or buy phases.

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
------------------------------------------------------------------------------
