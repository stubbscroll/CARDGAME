-- player selects any number of cards from hand
-- all selected cards are discarded, and player draws a card for each card
-- discarded
--
-- pseudocode
--
-- list=playerselectsfromhand(0,999)
-- for i=0;i<list.length;i++
--   discard(i)
-- add_card(list.length)

function on_setup()
  cards=10
  pileid=get_piles()
  set_piles(pileid+1)
  set_pile_cards(pileid,cards)
  for i=0,cards-1,1 do
    set_pile_card(pileid,i,thiscardid)
  end
end

function post_setup()
end
