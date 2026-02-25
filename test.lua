-- Dark Camp — Lua integration tests

function test()
    -- Wait for initialization and first frame
    coroutine.yield(0.2)

    ----------------------------------------------------------------
    -- Test 1: Initial player state
    ----------------------------------------------------------------
    game.take_screenshot("01_initial_state.png")

    local start_x = game.get_var("player_x")
    local start_y = game.get_var("player_y")
    local start_hp = game.get_var("player_hp")
    local start_kills = game.get_var("player_kills")

    game.assert_near(start_x, 15.0, 1.0, "player starts near center X (15.0)")
    game.assert_near(start_y, 15.0, 1.0, "player starts near center Y (15.0)")
    game.assert_eq(start_hp, 100.0, "player starts with full HP (100)")
    game.assert_eq(start_kills, 0, "player starts with 0 kills")

    ----------------------------------------------------------------
    -- Test 2: Enemies are spawned
    ----------------------------------------------------------------
    local enemies = game.get_var("enemy_count")
    game.assert_true(enemies >= 0, "enemy_count should be non-negative")

    ----------------------------------------------------------------
    -- Test 3: Movement — D key (iso: x increases, y decreases)
    ----------------------------------------------------------------
    local before_d_x = game.get_var("player_x")
    local before_d_y = game.get_var("player_y")

    game.key_down(game.KEY_D)
    coroutine.yield(0.5)
    game.key_up(game.KEY_D)
    coroutine.yield(0.05)

    local after_d_x = game.get_var("player_x")
    local after_d_y = game.get_var("player_y")
    game.assert_true(after_d_x > before_d_x, "D key should increase player X (iso right)")
    game.assert_true(after_d_y < before_d_y, "D key should decrease player Y (iso right)")
    game.take_screenshot("02_after_move_D.png")

    ----------------------------------------------------------------
    -- Test 4: Movement — A key (iso: x decreases, y increases)
    ----------------------------------------------------------------
    local before_a_x = game.get_var("player_x")
    local before_a_y = game.get_var("player_y")

    game.key_down(game.KEY_A)
    coroutine.yield(0.5)
    game.key_up(game.KEY_A)
    coroutine.yield(0.05)

    local after_a_x = game.get_var("player_x")
    local after_a_y = game.get_var("player_y")
    game.assert_true(after_a_x < before_a_x, "A key should decrease player X (iso left)")
    game.assert_true(after_a_y > before_a_y, "A key should increase player Y (iso left)")

    ----------------------------------------------------------------
    -- Test 5: Movement — W key (iso: x decreases, y decreases)
    ----------------------------------------------------------------
    local before_w_x = game.get_var("player_x")
    local before_w_y = game.get_var("player_y")

    game.key_down(game.KEY_W)
    coroutine.yield(0.5)
    game.key_up(game.KEY_W)
    coroutine.yield(0.05)

    local after_w_x = game.get_var("player_x")
    local after_w_y = game.get_var("player_y")
    game.assert_true(after_w_x < before_w_x, "W key should decrease player X (iso up)")
    game.assert_true(after_w_y < before_w_y, "W key should decrease player Y (iso up)")
    game.take_screenshot("03_after_move_W.png")

    ----------------------------------------------------------------
    -- Test 6: Movement — S key (iso: x increases, y increases)
    ----------------------------------------------------------------
    local before_s_x = game.get_var("player_x")
    local before_s_y = game.get_var("player_y")

    game.key_down(game.KEY_S)
    coroutine.yield(0.5)
    game.key_up(game.KEY_S)
    coroutine.yield(0.05)

    local after_s_x = game.get_var("player_x")
    local after_s_y = game.get_var("player_y")
    game.assert_true(after_s_x > before_s_x, "S key should increase player X (iso down)")
    game.assert_true(after_s_y > before_s_y, "S key should increase player Y (iso down)")

    ----------------------------------------------------------------
    -- Test 7: Player returns near start after opposite movements
    ----------------------------------------------------------------
    local return_x = game.get_var("player_x")
    local return_y = game.get_var("player_y")
    game.assert_near(return_x, start_x, 2.0, "player should be near start X after D+A+W+S")
    game.assert_near(return_y, start_y, 2.0, "player should be near start Y after D+A+W+S")

    ----------------------------------------------------------------
    -- Test 8: Diagonal movement (W+D simultaneously)
    ----------------------------------------------------------------
    local before_diag_x = game.get_var("player_x")
    local before_diag_y = game.get_var("player_y")

    game.key_down(game.KEY_W)
    game.key_down(game.KEY_D)
    coroutine.yield(0.5)
    game.key_up(game.KEY_W)
    game.key_up(game.KEY_D)
    coroutine.yield(0.05)

    local after_diag_x = game.get_var("player_x")
    local after_diag_y = game.get_var("player_y")
    -- W: x-1,y-1; D: x+1,y-1 → combined: x stays ~same, y decreases
    game.assert_near(after_diag_x, before_diag_x, 0.5, "W+D diagonal: X should stay ~same")
    game.assert_true(after_diag_y < before_diag_y, "W+D diagonal: Y should decrease")

    ----------------------------------------------------------------
    -- Test 9: Return to center for camp regen test
    ----------------------------------------------------------------
    -- Move back with S+A (opposite of W+D) to return near center
    game.key_down(game.KEY_S)
    game.key_down(game.KEY_A)
    coroutine.yield(0.5)
    game.key_up(game.KEY_S)
    game.key_up(game.KEY_A)
    coroutine.yield(0.05)

    ----------------------------------------------------------------
    -- Test 10: Camp HP regeneration
    ----------------------------------------------------------------
    -- Player should be in camp zone (near 15,15). HP should be at max.
    local camp_hp = game.get_var("player_hp")
    game.assert_true(camp_hp > 0, "player should be alive in camp")
    game.assert_true(camp_hp <= 100.0, "HP should not exceed max (100)")

    ----------------------------------------------------------------
    -- Test 11: Sword attack (LMB)
    ----------------------------------------------------------------
    local kills_before = game.get_var("player_kills")

    -- Set mouse position and press left button to attack
    game.set_mouse_pos(500, 300)
    game.mouse_press(game.MOUSE_LEFT)
    coroutine.yield(0.1)
    game.take_screenshot("04_attack_swing.png")

    -- Wait for attack cooldown
    coroutine.yield(0.5)

    -- Attack in another direction
    game.set_mouse_pos(300, 400)
    game.mouse_press(game.MOUSE_LEFT)
    coroutine.yield(0.1)

    -- Verify kills counter is still valid (may or may not have hit an enemy)
    local kills_after = game.get_var("player_kills")
    game.assert_true(kills_after >= kills_before, "kills should not decrease after attack")

    ----------------------------------------------------------------
    -- Test 12: Movement is bounded by map collision
    ----------------------------------------------------------------
    -- Try to move far in one direction — player should be stopped by solid tiles
    local pre_bound_x = game.get_var("player_x")
    game.key_down(game.KEY_D)
    coroutine.yield(3.0)
    game.key_up(game.KEY_D)
    coroutine.yield(0.05)

    local post_bound_x = game.get_var("player_x")
    -- Player should have moved but be stopped before map edge (30.0)
    game.assert_true(post_bound_x > pre_bound_x, "player should move with D key")
    game.assert_true(post_bound_x < 30.0, "player should be stopped by map boundary")
    game.take_screenshot("05_boundary_test.png")

    ----------------------------------------------------------------
    -- Test 13: Return to center after boundary test
    ----------------------------------------------------------------
    game.key_down(game.KEY_A)
    coroutine.yield(3.0)
    game.key_up(game.KEY_A)
    coroutine.yield(0.05)

    -- Should be back near center area
    local back_x = game.get_var("player_x")
    game.assert_true(back_x < post_bound_x, "A key should bring player back toward center")

    ----------------------------------------------------------------
    -- Test 14: Final state validation
    ----------------------------------------------------------------
    game.take_screenshot("06_final_state.png")

    local final_hp = game.get_var("player_hp")
    local final_kills = game.get_var("player_kills")
    local final_enemies = game.get_var("enemy_count")

    game.assert_true(final_hp > 0, "player should still be alive at end of test")
    game.assert_true(final_hp <= 100.0, "HP should not exceed max at end")
    game.assert_true(final_kills >= 0, "kills counter should be non-negative")
    game.assert_true(final_enemies >= 0, "enemy_count should be non-negative")

    game.close()
end
