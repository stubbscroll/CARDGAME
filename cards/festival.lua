function victory_points()
  return 0
end

function money_cost()
  return 5
end

function potion_cost()
  return 0
end

function on_play()
  add_action(2)
  add_buy(1)
  add_money(2)
end

function can_buy()
  return 1
end

function on_setup()
  cards=10
  pileid=get_piles()
  set_piles(pileid+1)
  init_pile(pileid)
  set_pile_supply(pileid,1)
  set_pile_cards(pileid,cards)
  for i=0,cards-1,1 do
    set_pile_card(pileid,i,thiscardid)
  end
end

function post_setup()
end
