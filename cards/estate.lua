function on_play()
end

function victory_points()
  return 1
end

function money_cost()
  return 2
end

function potion_cost()
  return 0
end

function on_setup()
  if get_players()==2 then
    cards=8
  else
    cards=12
  end
  pileid=get_piles()
  set_piles(pileid+1)
  set_pile_cards(pileid,cards)
  for i=0,cards-1,1 do
    set_pile_card(pileid,i,thiscardid)
  end
end

function post_setup()
end
