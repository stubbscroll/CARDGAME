function on_play()
  add_action(1)
  add_card(1)
  add_buy(1)
  add_money(2)
end

function victory_points()
  return 0
end

function money_cost()
  return 6
end

function potion_cost()
  return 0
end

function can_buy()
  -- cannot buy if there is copper in play
  cur=get_currentplayer()
  n=get_player_playarean(cur)
  -- go through each card in current player's play area and check for copper
  for i=0,n-1,1 do
    id=get_player_playarea(cur,i)
    name=get_card_fullname(id)
    if name=="Copper" then
      return 0
    end
  end
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
