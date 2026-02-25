# Архитектура: Diablo 2 прототип на C/raylib

## Паттерны из кодовой базы

**Найденные конвенции** (из анализа `src/main.c`, `CMakeLists.txt`):

- **Стандарт**: C23, `-pedantic-errors`, `-Wall -Werror -Wextra` — никаких предупреждений
- **Санитайзеры**: AddressSanitizer + UBSanitizer — строгое управление памятью, нет UB
- **Сборка**: `file(GLOB_RECURSE SOURCES src/*.c)` — все `.c` файлы в `src/` подхватываются автоматически
- **raylib_tester.h**: STB-стиль, `#define RAYLIB_TESTER_IMPLEMENTATION` только в `main.c`
- **Ввод**: ОБЯЗАТЕЛЬНО через `RltIsKeyDown()`/`RltIsKeyPressed()`/`RltIsMouseButtonPressed()`, не через raw raylib
- **Порядок цикла**: обновление → `RltClearSimOneshot()` → `BeginDrawing()` ... `EndDrawing()` → `RltUpdateScriptRunner()`
- **Переменные для тестов**: `RltRegisterVar()` после инициализации объектов
- **Именование функций**: `VerbNoun` или `VerbModule` (например `InitGame`, `UpdatePlayer`, `RenderMap`)
- **Именование переменных**: `snake_case` для полей структур

---

## Архитектурное решение

Классическая **ECS-lite** архитектура на C без динамической памяти:
- Все сущности в статических массивах (`GameState`)
- Модули как `.h/.c` пары с чёткими границами ответственности
- Никакого `malloc` — все объекты заранее выделены в `GameState`
- Изометрическая проекция 2:1 с `Camera2D` raylib для pan/zoom
- AI-стейт машина в `enemy.c`, боевая логика в `combat.c`

---

## Структура файлов

```
src/
  main.c          - точка входа, игровой цикл, интеграция с raylib_tester
  game.h          - все общие типы, константы, struct GameState (NO .c)
  player.h/c      - Player: движение WASD, атака мышью, кулдауны
  enemy.h/c       - Enemy: AI стейт машина (IDLE/PATROL/CHASE/ATTACK), спавн
  map.h/c         - TileType, процедурная генерация 30x30, проверка коллизий
  combat.h/c      - hitbox дуга атаки, damage apply, проверка попаданий
  render.h/c      - вся отрисовка: тайлы, враги, игрок, UI; изо-проекция
  raylib_tester.h - тестовый фреймворк (НЕ МЕНЯТЬ)
docs/
  ARCHITECTURE.md - этот файл
```

---

## Ключевые структуры данных (`game.h`)

```c
#pragma once
#include <stdbool.h>

#define MAP_WIDTH  30
#define MAP_HEIGHT 30
#define TILE_SIZE  32
#define MAX_ENEMIES 20

// Tile size в мировых координатах (пиксели не используются в логике)
#define TILE_WORLD_SIZE 1.0f

// Изо-проекция: мир → экран
#define ISO_SCALE_X  (TILE_SIZE / 2)   // 16
#define ISO_SCALE_Y  (TILE_SIZE / 4)   // 8

typedef enum {
    TILE_GRASS,   // проходимо, зелёная трава
    TILE_DIRT,    // проходимо, грунт
    TILE_GRAVE,   // НЕПРОХОДИМО, надгробие
    TILE_CAMP,    // проходимо, зона лагеря (враги не заходят)
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
    float x, y;           // позиция в мировых координатах (тайловая сетка)
    float hp, max_hp;
    float speed;          // тайлов/сек
    float attack_range;   // радиус атаки (в тайлах)
    float attack_arc;     // угол дуги атаки (радианы)
    float attack_cooldown; // секунды между атаками
    float attack_timer;   // оставшееся время до следующей атаки
    int   damage;
    int   kills;
    bool  attacking;
    float attack_angle;   // направление атаки (радианы)
} Player;

typedef struct {
    EnemyType type;
    float x, y;
    float hp, max_hp;
    float speed;
    int   damage;
    float aggro_radius;   // радиус обнаружения игрока
    float attack_radius;  // радиус ближней атаки
    float attack_cooldown;
    float attack_timer;
    bool  alive;
    float respawn_timer;  // >0 когда мёртв, ожидает воскрешения
    float spawn_x, spawn_y; // точка появления
    float patrol_target_x, patrol_target_y;
    AiState state;
} Enemy;

typedef struct {
    Player player;
    Enemy  enemies[MAX_ENEMIES];
    int    enemy_count;                     // живых врагов
    TileType map[MAP_HEIGHT][MAP_WIDTH];
    bool   running;
    float  camera_x, camera_y;             // позиция камеры (мировые координаты)
} GameState;
```

---

## Компонент: `game.h` (только заголовок)

**Ответственность**: общие типы и константы, доступные всем модулям.

**Нет `.c` файла** — только определения типов, `#define` константы, `enum` и `struct`.

**Зависимости**: `<stdbool.h>` (без raylib, чтобы логика была независима от рендера).

---

## Компонент: `map.h / map.c`

**Ответственность**: генерация тайловой карты, запросы к тайлам, проверка проходимости.

### Интерфейс (`map.h`):

```c
#pragma once
#include "game.h"

void MapInit(GameState* state);
bool MapIsSolid(const GameState* state, float world_x, float world_y);
bool MapIsCamp(const GameState* state, float world_x, float world_y);
TileType MapGetTile(const GameState* state, int tile_x, int tile_y);
```

### Реализация (`map.c`):

Процедурная генерация (никаких файлов, никакого `malloc`):

```
Зоны по расстоянию от центра (15, 15):
  dist <= 2  → TILE_CAMP  (зона лагеря 13-17, 13-17)
  dist <= 8  → TILE_GRASS (кольцо травы)
  dist > 8   → TILE_GRAVE (внешнее кольцо кладбища)
  + случайный TILE_DIRT патч внутри травы
```

`MapIsSolid()` — возвращает `true` для `TILE_GRAVE`.

**Детерминированность**: никакого `srand()` — паттерн надгробий вычисляется по координатам тайла (`(tile_x * 7 + tile_y * 13) % 3 == 0`).

---

## Компонент: `player.h / player.c`

**Ответственность**: инициализация игрока, обработка ввода движения, атаки, обновление таймеров.

### Интерфейс (`player.h`):

```c
#pragma once
#include "game.h"

void PlayerInit(Player* player);
void PlayerHandleInput(GameState* state, float delta);
void PlayerUpdate(GameState* state, float delta);
```

### Реализация:

**Движение (WASD)**:
- Изометрическое управление: W/S = диагональ вверх/вниз, A/D = диагональ влево/вправо
- Нормализация вектора при диагональном движении
- Проверка `MapIsSolid()` для следующей позиции (коллизия AABB ~0.4 тайла)
- `RltIsKeyDown(KEY_W)` / `KEY_A` / `KEY_S` / `KEY_D`

**Атака (LMB)**:
- `RltIsMouseButtonPressed(MOUSE_BUTTON_LEFT)` → вычислить угол к курсору мыши
- Преобразовать экранные координаты мыши → мировые (обратная изо-проекция)
- Установить `player->attacking = true`, `player->attack_angle = atan2f(dy, dx)`
- `attack_timer = attack_cooldown` (блокирует повторную атаку)

**Таймеры**:
- `attack_timer -= delta`, при достижении 0: `attacking = false`

---

## Компонент: `enemy.h / enemy.c`

**Ответственность**: инициализация врагов по типу, обновление AI, спавн/ре-спавн.

### Интерфейс (`enemy.h`):

```c
#pragma once
#include "game.h"

void EnemiesInit(GameState* state);
void EnemiesUpdate(GameState* state, float delta);
```

### Реализация:

**Инициализация**: расставляем врагов в зоне кладбища (вне лагеря):
- 8 скелетов (быстрые, слабые)
- 8 зомби (медленные, крепкие)
- 4 лича (средние, магические)

**Параметры по типу**:
| Тип       | HP  | speed | damage | aggro | attack_r | cooldown | respawn |
|-----------|-----|-------|--------|-------|----------|----------|---------|
| SKELETON  | 30  | 3.5   | 5      | 8.0   | 1.0      | 1.0s     | 10s     |
| ZOMBIE    | 80  | 1.5   | 12     | 6.0   | 1.2      | 1.5s     | 15s     |
| LICH      | 50  | 2.5   | 20     | 10.0  | 1.5      | 2.0s     | 20s     |

**AI Стейт машина** (на каждый кадр для живого врага):

```
AI_IDLE:
  → Если dist(enemy, player) < aggro_radius → AI_CHASE
  → Иначе раз в 3 секунды: выбрать patrol_target в радиусе 3 тайла от spawn

AI_PATROL:
  → Двигаться к patrol_target (speed * 0.5)
  → Если достигнут target → AI_IDLE
  → Если dist(enemy, player) < aggro_radius → AI_CHASE

AI_CHASE:
  → Двигаться к игроку (полная скорость)
  → Не заходить в TILE_CAMP (MapIsCamp проверка следующего тайла)
  → Если dist(enemy, player) <= attack_radius → AI_ATTACK
  → Если dist(enemy, player) > aggro_radius * 1.5 → AI_IDLE (потерял игрока)

AI_ATTACK:
  → attack_timer -= delta
  → Если attack_timer <= 0: нанести урон игроку, attack_timer = attack_cooldown
  → Если dist(enemy, player) > attack_radius → AI_CHASE
```

**Ре-спавн**:
- При `hp <= 0`: `alive = false`, `respawn_timer = RESPAWN_TIME_BY_TYPE`
- `respawn_timer -= delta`
- При `respawn_timer <= 0`: `alive = true`, `hp = max_hp`, `x = spawn_x`, `y = spawn_y`, `state = AI_IDLE`
- `enemy_count` = количество живых врагов (пересчитывается после Update)

---

## Компонент: `combat.h / combat.c`

**Ответственность**: проверка попаданий атаки игрока по врагам, применение урона.

### Интерфейс (`combat.h`):

```c
#pragma once
#include "game.h"

void CombatUpdate(GameState* state);
```

### Реализация:

**Атака игрока** (вызывается когда `player.attacking == true`):

```c
// Дуга атаки: сектор круга
float ATTACK_RANGE = 1.5f; // тайлов
float ATTACK_ARC   = 2.09f; // ~120 градусов в радианах

for each alive enemy:
    float dx = enemy.x - player.x;
    float dy = enemy.y - player.y;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist > ATTACK_RANGE) continue;

    float angle = atan2f(dy, dx);
    float angle_diff = fabsf(AngleDiff(angle, player.attack_angle));
    if (angle_diff > ATTACK_ARC / 2.0f) continue;

    enemy.hp -= player.damage;
    if (enemy.hp <= 0):
        enemy.alive = false;
        player.kills++;
```

`AngleDiff()` — корректная разница углов с учётом wrap-around (-π, π).

**Внимание**: урон применяется только в ОДИН кадр (пока `attacking` переходит из `false` в `true`), не каждый кадр пока анимация активна — отслеживается через `attack_applied` флаг в `Player`.

**Урон по игроку**: применяется в `enemy.c` (при `AI_ATTACK` и `attack_timer <= 0`), а не в `combat.c`. `combat.c` отвечает только за атаку игрока.

---

## Компонент: `render.h / render.c`

**Ответственность**: вся отрисовка — тайлы, враги, игрок, UI. Изометрическая проекция.

### Интерфейс (`render.h`):

```c
#pragma once
#include "game.h"
#include <raylib.h>

// Изо-проекция: мировые координаты → экранные
Vector2 WorldToScreen(float world_x, float world_y, float cam_x, float cam_y);

void RenderMap(const GameState* state);
void RenderEnemies(const GameState* state);
void RenderPlayer(const GameState* state);
void RenderUI(const GameState* state);
```

### Изометрическая проекция:

```c
// Центр экрана как offset камеры
#define SCREEN_CENTER_X 400
#define SCREEN_CENTER_Y 300

Vector2 WorldToScreen(float wx, float wy, float cam_x, float cam_y) {
    float rx = wx - cam_x;
    float ry = wy - cam_y;
    return (Vector2){
        .x = (rx - ry) * ISO_SCALE_X + SCREEN_CENTER_X,
        .y = (rx + ry) * ISO_SCALE_Y + SCREEN_CENTER_Y,
    };
}

// Обратная (для мыши → мировые координаты):
void ScreenToWorld(float sx, float sy, float cam_x, float cam_y,
                   float* wx, float* wy) {
    float rx = sx - SCREEN_CENTER_X;
    float ry = sy - SCREEN_CENTER_Y;
    *wx = (rx / ISO_SCALE_X + ry / ISO_SCALE_Y) / 2.0f + cam_x;
    *wy = (ry / ISO_SCALE_Y - rx / ISO_SCALE_X) / 2.0f + cam_y;
}
```

### Порядок отрисовки (Painter's Algorithm):

```
1. ClearBackground(DARKGRAY)
2. RenderMap: тайл за тайлом, row by row (Y растёт вниз в изо-пространстве)
3. RenderEnemies: мёртвые враги (кости) → затем живые, сортировка по (x+y)
4. RenderPlayer
5. RenderUI: HP bar, счётчик убийств, FPS
```

### Визуализация (процедурная, без текстур):

| Объект    | Функция raylib                           | Цвет              |
|-----------|------------------------------------------|-------------------|
| TILE_GRASS | `DrawRectangleV` (ромб из 2 треугольников) | GREEN / DARKGREEN |
| TILE_DIRT  | аналогично                               | BROWN             |
| TILE_GRAVE | `DrawRectangleV` + вертикальный прямоугольник | DARKGRAY + GRAY  |
| TILE_CAMP  | `DrawRectangleV` + `DrawCircle` костёр   | BEIGE + ORANGE    |
| Player     | `DrawCircleV` (radius 12)                | BLUE              |
| SKELETON   | `DrawCircleV` (radius 10)                | WHITE             |
| ZOMBIE     | `DrawCircleV` (radius 14)                | GREEN             |
| LICH       | `DrawCircleV` (radius 11)                | PURPLE            |
| HP bar     | `DrawRectangleRec` (2 слоя)              | RED / DARKGRAY    |

Атака игрока: `DrawCircleSector()` (дуга) пока `player.attacking == true`.

---

## Игровой цикл (`main.c`)

```c
#define RAYLIB_TESTER_IMPLEMENTATION
#include "raylib_tester.h"
#include "game.h"
#include "player.h"
#include "enemy.h"
#include "map.h"
#include "combat.h"
#include "render.h"

int main(int argc, char* argv[]) {
    // 1. Инициализация окна
    InitWindow(800, 600, "Diablo2 Prototype");
    SetTargetFPS(60);

    // 2. Инициализация игры
    static GameState state = {0};
    MapInit(&state);
    PlayerInit(&state.player);
    EnemiesInit(&state);
    state.running = true;
    state.camera_x = state.player.x;
    state.camera_y = state.player.y;

    // 3. Script runner (опционально)
    RltScriptRunner* runner = NULL;
    if (argc > 1) {
        runner = RltInitScriptRunner(argv[1]);
        if (runner == NULL) { CloseWindow(); return 1; }
    }

    // 4. Регистрация переменных для тестов
    RltRegisterVar("player_x",    &state.player.x,      RLT_VAR_FLOAT);
    RltRegisterVar("player_y",    &state.player.y,      RLT_VAR_FLOAT);
    RltRegisterVar("player_hp",   &state.player.hp,     RLT_VAR_FLOAT);
    RltRegisterVar("enemy_count", &state.enemy_count,   RLT_VAR_INT);
    RltRegisterVar("player_kills",&state.player.kills,  RLT_VAR_INT);

    // 5. Игровой цикл
    while (!WindowShouldClose() && !RltShouldClose() && state.running) {
        float delta = GetFrameTime();

        // a. Обновление
        PlayerHandleInput(&state, delta);
        PlayerUpdate(&state, delta);
        EnemiesUpdate(&state, delta);
        CombatUpdate(&state);

        // b. Камера следует за игроком
        state.camera_x = state.player.x;
        state.camera_y = state.player.y;

        // c. Очистить oneshot input (ПОСЛЕ логики, ДО рендера)
        RltClearSimOneshot();

        // d. Рендер
        BeginDrawing();
        RenderMap(&state);
        RenderEnemies(&state);
        RenderPlayer(&state);
        RenderUI(&state);
        DrawFPS(0, 0);
        EndDrawing();

        // e. Тестовый раннер
        if (runner != NULL) {
            RltUpdateScriptRunner(runner);
        }
    }

    // 6. Cleanup
    bool test_failed = (runner != NULL) && RltScriptRunnerHadError(runner);
    if (runner != NULL) RltCloseScriptRunner(runner);
    CloseWindow();
    return test_failed ? 1 : 0;
}
```

---

## Поток данных

```
Ввод (клавиатура/мышь/Lua script)
        │
        ▼
PlayerHandleInput(&state, delta)
  - Rlt*Key/Mouse → player.velocity, player.attacking, player.attack_angle
        │
        ▼
PlayerUpdate(&state, delta)
  - Применить движение → player.x, player.y (с проверкой MapIsSolid)
  - Обновить attack_timer
        │
        ▼
EnemiesUpdate(&state, delta)
  - Для каждого живого врага: AI стейт машина → enemy.x/y, enemy.state
  - Урон по игроку при AI_ATTACK → player.hp
  - Ре-спавн мёртвых врагов
  - Пересчёт enemy_count
        │
        ▼
CombatUpdate(&state)
  - Если player.attacking && !attack_applied:
    - Проверить дугу против всех врагов
    - Применить player.damage → enemy.hp, kills++
    - attack_applied = true
  - Если !player.attacking: attack_applied = false
        │
        ▼
Камера: state.camera_x/y = player.x/y
        │
        ▼
RltClearSimOneshot()
        │
        ▼
RenderMap → RenderEnemies → RenderPlayer → RenderUI
  - WorldToScreen(x, y, camera_x, camera_y) для каждого объекта
  - Painter's algorithm: тайлы → враги → игрок → UI
        │
        ▼
RltUpdateScriptRunner  ← проверяет RltRegisterVar-переменные
```

---

## Последовательность реализации

- [ ] **Фаза 1: Каркас**
  - [ ] Создать `game.h` с типами и константами
  - [ ] Создать `map.h/c` с `MapInit` и заглушкой `MapIsSolid`
  - [ ] Создать `render.h/c` с `WorldToScreen` и `RenderMap`
  - [ ] Обновить `main.c`: инициализация + цикл с рендером карты
  - [ ] Убедиться, что `make build` работает

- [ ] **Фаза 2: Игрок**
  - [ ] Создать `player.h/c`: инициализация, движение WASD
  - [ ] Добавить `RenderPlayer` в `render.c`
  - [ ] Зарегистрировать `player_x`, `player_y` в `main.c`
  - [ ] Тест: игрок движется по карте

- [ ] **Фаза 3: Враги**
  - [ ] Создать `enemy.h/c`: инициализация по типу, обновление AI
  - [ ] Добавить `RenderEnemies` в `render.c`
  - [ ] Зарегистрировать `enemy_count` в `main.c`
  - [ ] Тест: враги агрятся и преследуют

- [ ] **Фаза 4: Боевая система**
  - [ ] Создать `combat.h/c`: дуга атаки, применение урона
  - [ ] Добавить атаку игрока (LMB)
  - [ ] Зарегистрировать `player_hp`, `player_kills`
  - [ ] Добавить `RenderUI` (HP bar, kills)
  - [ ] Тест: игрок убивает врагов, враги убивают игрока

- [ ] **Фаза 5: Полировка**
  - [ ] Ре-спавн врагов
  - [ ] Визуальная дуга атаки через `DrawCircleSector`
  - [ ] Анимация костра на лагере
  - [ ] `make tidy` — нет ошибок clang-tidy

---

## Критические детали

### Управление памятью
- **Никакого `malloc`** — все массивы статические в `GameState`
- `static GameState state = {0}` в `main.c` — нулевая инициализация
- `RltInitScriptRunner` использует `static RltScriptRunner` внутри — только один экземпляр

### Строгая компиляция
- Все параметры должны использоваться или явно кастоваться к `(void)`
- Нет неявных преобразований (`float` vs `int` — всегда явный каст)
- Designated initializers для структур: `(Player){ .x = 15.0f, .y = 15.0f, ... }`
- Константы только через `#define` или `enum`, не через глобальные переменные

### Порядок include в `.c` файлах
```c
// В любом .c файле (например player.c):
#include "player.h"   // собственный заголовок первым
#include "game.h"     // общие типы
#include "map.h"      // зависимости
#include <raylib.h>   // внешние библиотеки последними
#include <math.h>
```

### Тестируемость через Lua
Зарегистрированные переменные позволяют Lua-скриптам:
```lua
-- test.lua
function test()
    -- Ждём пока враги активны
    coroutine.yield(0.5)

    local hp = game.get_var("player_hp")
    game.assert_true(hp > 0, "player should be alive")

    -- Симулируем движение вправо
    game.key_down(game.KEY_D)
    coroutine.yield(1.0)
    game.key_up(game.KEY_D)

    local x_after = game.get_var("player_x")
    game.assert_true(x_after > 15.0, "player should have moved right")

    game.close()
end
```

### Коллизии
- Проверяем 4 угла AABB игрока (±0.4 тайла) при движении
- Враги: проверяем только центр (упрощённо — они могут перекрываться)
- Лагерь: `MapIsCamp()` проверяется в AI движении врагов

### Производительность
- 20 врагов × простые операции = O(400) для combat check — trivial
- Карта 30×30 = 900 тайлов DrawRectangle — приемлемо для 60fps
- Нет sorting врагов для Painter's (достаточно row-by-row тайлового рендера)
