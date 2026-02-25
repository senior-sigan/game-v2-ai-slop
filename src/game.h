#pragma once
#include <stdbool.h>

#define MAP_WIDTH 30
#define MAP_HEIGHT 30
#define TILE_SIZE 32
#define MAX_ENEMIES 20

#define TILE_WORLD_SIZE 1.0F

#define ISO_SCALE_X (TILE_SIZE / 2)
#define ISO_SCALE_Y (TILE_SIZE / 4)

#define SCREEN_CENTER_X 400
#define SCREEN_CENTER_Y 300

#define PLAYER_SPEED 4.0F
#define PLAYER_MAX_HP 100.0F
#define PLAYER_DAMAGE 25
#define PLAYER_ATTACK_RANGE 1.5F
#define PLAYER_ATTACK_ARC 2.09F
#define PLAYER_ATTACK_COOLDOWN 0.5F
#define CAMP_REGEN_RATE 5.0F

typedef enum {
  TILE_GRASS,
  TILE_DIRT,
  TILE_GRAVE,
  TILE_CAMP,
} TileType;

typedef enum {
  ENEMY_SKELETON,
  ENEMY_ZOMBIE,
  ENEMY_LICH,
} EnemyType;

typedef enum {
  AI_IDLE,
  AI_PATROL,
  AI_CHASE,
  AI_ATTACK,
} AiState;

typedef struct {
  int current_frame;
  float frame_timer;
  float frame_duration;
  int frame_count;
  int kind;
} Animation;

typedef struct {
  float x, y;
  float vx, vy;
  float hp, max_hp;
  float speed;
  float attack_range;
  float attack_arc;
  float attack_cooldown;
  float attack_timer;
  int damage;
  int kills;
  bool attacking;
  bool attack_applied;
  float attack_angle;
  Animation anim;
} Player;

typedef struct {
  EnemyType type;
  float x, y;
  float hp, max_hp;
  float speed;
  int damage;
  float aggro_radius;
  float attack_radius;
  float attack_cooldown;
  float attack_timer;
  bool alive;
  float respawn_timer;
  float spawn_x, spawn_y;
  float patrol_target_x, patrol_target_y;
  AiState state;
  Animation anim;
} Enemy;

typedef struct {
  Player player;
  Enemy enemies[MAX_ENEMIES];
  int enemy_count;
  TileType map[MAP_HEIGHT][MAP_WIDTH];
  bool running;
  float camera_x, camera_y;
} GameState;
