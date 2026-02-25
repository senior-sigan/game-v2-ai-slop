local screenshot_interval = 0.5
local max_screenshots = 10
local screenshot_count = 0
local last_screenshot_time = 0

function update()
  local t = game.get_time()

  if screenshot_count >= max_screenshots then
    print("All " .. max_screenshots .. " screenshots taken, closing game.")
    game.close()
    return
  end

  if t - last_screenshot_time >= screenshot_interval then
    screenshot_count = screenshot_count + 1
    local filename = string.format("screenshot_%02d.png", screenshot_count)
    print("Taking screenshot " .. screenshot_count .. "/" .. max_screenshots .. ": " .. filename)
    game.take_screenshot(filename)
    last_screenshot_time = t
  end
end
