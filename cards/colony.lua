function victory_points()
  return 10
end

function money_cost()
  return 11
end

function potion_cost()
  return 0
end

function can_buy()
  return 1
end

function on_setup()
  if get_players()==2 then
    cards=8
  else
    cards=12
  end
  pileid=get_piles()
  set_piles(pileid+1)
  init_pile(pileid)
  set_pile_supply(pileid,1)
  set_pile_cards(pileid,cards)
  for i=0,cards-1,1 do
    set_pile_card(pileid,i,thisgroupid)
  end
end

function post_setup()
end
