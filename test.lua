function test()
  for i = 1, 10 do
    game.take_screenshot(string.format("screenshot_%02d.png", i))
    coroutine.yield(0.5)
  end
  game.close()
end
