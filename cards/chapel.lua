-- ask player to choose between 0 and 4 cards from hand
-- loop through each of them and put them in trash
--
-- pseudocode in non-lua follows
--
-- list = selectcardsfromhand(0,4)
-- for i=0 to list.length
--   trash(list[i].card)

function victory_points()
  return 0
end

function money_cost()
  return 2
end

function potion_cost()
  return 0
end

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
