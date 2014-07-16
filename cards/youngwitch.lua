function on_play()
  add_card(2)
  -- curse to each player who doesn't reveal bane card
  -- discard 2 cards
end

function victory_points()
  return 0
end

function money_cost()
  return 4
end

function potion_cost()
  return 0
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
  -- find a previously unpicked kingdom card with cost 2 or 3
  math.randomseed(os.time())
  baneid=-1
  n=get_groups()
  -- separate pass to check if eligible card exists 
  has=0
  for i=0,n-1,1 do
    if get_group_taken(i)==0 and get_group_iskingdom(i)==1 and get_card_potion_cost(get_group_card(i,0))==0 then
      cost=get_card_money_cost(get_group_card(i,0))
      if cost==2 or cost==3 then has=1 end
    end
  end
  if has==0 then
    print("ERROR (young witch): couldn't find unpicked kingdom card with cost 2 or 3")
  end
  -- now, find random eligible card
  while baneid==-1 do
    i=math.random(0,n-1)
    if get_group_taken(i)==0 and get_group_iskingdom(i)==1 and get_card_potion_cost(get_group_card(i,0))==0 then
      cost=get_card_money_cost(get_group_card(i,0))
      if cost==2 or cost==3 then baneid=i end
    end
  end
  -- add the pile
  add_pile(baneid)
  -- mark pile as bane in supply
  n=get_piles()
  for i=0,n-1,1 do
    p=get_pile_card(i,0)
    if get_card_groupid(p)==baneid then
    set_pile_flag(i,0,1)
    end
  end
end
