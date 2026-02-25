function test()
  coroutine.yield(0.1) -- let first frame render

  local start_x = game.get_var("rect_x")
  print("Start rect_x = " .. start_x)
  game.take_screenshot("01_start.png")

  -- Hold RIGHT arrow for 2 seconds
  game.key_down(game.KEY_RIGHT)
  coroutine.yield(2.0)
  game.key_up(game.KEY_RIGHT)

  local after_right_x = game.get_var("rect_x")
  print("After right rect_x = " .. after_right_x)
  assert(after_right_x > start_x, "rect should have moved right")
  game.take_screenshot("02_after_right.png")

  -- Hold LEFT arrow for 3 seconds
  game.key_down(game.KEY_LEFT)
  coroutine.yield(3.0)
  game.key_up(game.KEY_LEFT)

  local after_left_x = game.get_var("rect_x")
  print("After left rect_x = " .. after_left_x)
  assert(after_left_x < after_right_x, "rect should have moved left")
  game.take_screenshot("03_after_left.png")

  -- Move mouse and click
  game.set_mouse_pos(400, 300)
  game.mouse_press(game.MOUSE_LEFT)
  coroutine.yield(0.1)
  game.take_screenshot("04_mouse_click.png")

  print("All assertions passed!")
  game.close()
end
