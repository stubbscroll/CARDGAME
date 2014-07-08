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
