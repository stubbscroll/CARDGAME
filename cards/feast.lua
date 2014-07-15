function on_play()
--  trashthis()
--  gain_card(0,5)
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
