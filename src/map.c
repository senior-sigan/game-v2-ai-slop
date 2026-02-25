#include "map.h"

void MapInit(GameState* state) {
    int cx = MAP_WIDTH / 2;
    int cy = MAP_HEIGHT / 2;

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist_sq = dx * dx + dy * dy;

            if (dx >= -2 && dx <= 2 && dy >= -2 && dy <= 2) {
                state->map[y][x] = TILE_CAMP;
            } else if (dist_sq <= 64) {
                state->map[y][x] = TILE_GRASS;
            } else {
                if ((x * 7 + y * 13) % 5 == 0) {
                    state->map[y][x] = TILE_GRAVE;
                } else {
                    state->map[y][x] = TILE_DIRT;
                }
            }
        }
    }
}

TileType MapGetTile(const GameState* state, int tile_x, int tile_y) {
    if (tile_x < 0 || tile_x >= MAP_WIDTH || tile_y < 0 || tile_y >= MAP_HEIGHT) {
        return TILE_GRAVE;
    }
    return state->map[tile_y][tile_x];
}

bool MapIsSolid(const GameState* state, float world_x, float world_y) {
    if (world_x < 0.0f || world_x >= (float)MAP_WIDTH ||
        world_y < 0.0f || world_y >= (float)MAP_HEIGHT) {
        return true;
    }
    int tx = (int)world_x;
    int ty = (int)world_y;
    return MapGetTile(state, tx, ty) == TILE_GRAVE;
}

bool MapIsCamp(const GameState* state, float world_x, float world_y) {
    if (world_x < 0.0f || world_x >= (float)MAP_WIDTH ||
        world_y < 0.0f || world_y >= (float)MAP_HEIGHT) {
        return false;
    }
    int tx = (int)world_x;
    int ty = (int)world_y;
    return MapGetTile(state, tx, ty) == TILE_CAMP;
}
