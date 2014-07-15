function victory_points()
  return 0
end

function money_cost()
  return 6
end

function potion_cost()
  return 0
end

function on_play()
  add_money(2)
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
