function on_play()
  add_card(1)
  add_action(1)
  add_money(1)
end

function victory_points()
  return 0
end

function money_cost()
-- during the buy phase, this costs 2 less per action card you have in play,
-- but not less than 0
-- return 8 - 2*num
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
