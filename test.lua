function test()
  coroutine.yield(0.1) -- give some time for renderer
  -- Screenshot at start: rectangle in center
  game.take_screenshot("before.png")
  coroutine.yield(0.1)

  -- Hold RIGHT arrow for 2 seconds
  game.key_down(game.KEY_RIGHT)
  coroutine.yield(2.0)
  game.key_up(game.KEY_RIGHT)

  game.take_screenshot("after_right.png")
  coroutine.yield(0.1)

  -- Hold LEFT arrow for 3 seconds
  game.key_down(game.KEY_LEFT)
  coroutine.yield(3.0)
  game.key_up(game.KEY_LEFT)

  game.take_screenshot("after_left.png")
  coroutine.yield(0.1)

  game.close()
end
