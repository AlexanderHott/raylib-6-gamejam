#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_NO_ICONS
#include "raygui.h"
#include <math.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define ASSERT(condition, message) assert((condition) && (message))

#define STATIC_ASSERT_JOIN2(a, b) a##b
#define STATIC_ASSERT_JOIN(a, b) STATIC_ASSERT_JOIN2(a, b)
#define STATIC_ASSERT(condition, message)                                      \
  typedef char STATIC_ASSERT_JOIN(static_assertion_failed__##message##__line_, \
                                  __LINE__)[(condition) ? 1 : -1]

#define UNUSED(identifier) ((void)identifier)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

STATIC_ASSERT(sizeof(u8) == 1, u8_is_not_1_byte);
STATIC_ASSERT(sizeof(u16) == 2, u16_is_not_2_bytes);
STATIC_ASSERT(sizeof(u32) == 4, u32_is_not_4_bytes);
STATIC_ASSERT(sizeof(u64) == 8, u64_is_not_8_bytes);

STATIC_ASSERT(sizeof(i8) == 1, i8_is_not_1_byte);
STATIC_ASSERT(sizeof(i16) == 2, i16_is_not_2_bytes);
STATIC_ASSERT(sizeof(i32) == 4, i32_is_not_4_bytes);
STATIC_ASSERT(sizeof(i64) == 8, i64_is_not_8_bytes);

STATIC_ASSERT(sizeof(f32) == 4, f32_is_not_4_bytes);
STATIC_ASSERT(sizeof(f64) == 8, f32_is_not_8_bytes);

static const u32 screenWidth = 720;
static const u32 screenHeight = 720;

static const f32 trafficCarScale = 3.0f;

////////////////////////////////////////////////////////////////////////////////
/// Game
////////////////////////////////////////////////////////////////////////////////

#define TRAFFIC_CARS_MAX 16

typedef struct WorldVector3 {
  f32 x;
  f32 y;
  f32 z;
} WorldVector3;

typedef struct Camera25D {
  f32 centerX;
  f32 horizonY;
  f32 focalLength;
  f32 cameraHeight;
} Camera25D;

typedef struct TrafficCar {
  f32 x;
  f32 z;
  f32 width;
  f32 height;
  f32 length;
  Color color;
} TrafficCar;

typedef struct GameState {
  TrafficCar cars[TRAFFIC_CARS_MAX];
  u32 carCount;

  f32 roadScroll;
  f32 playerSpeed;
  f32 roadBottomY;
  f32 leftRoadLineX;
  f32 centerRoadLineX;
  f32 rightRoadLineX;
} GameState;

typedef struct Renderer {
  Camera25D camera;
  RenderTexture2D target;

  Texture2D background;
  Texture2D trafficCar;

  u32 screenWidth;
  u32 screenHeight;
} Renderer;

typedef struct AppState {
  GameState game;
  Renderer renderer;
  u32 frameCounter;
} AppState;

static AppState app = {0};

static Vector2 ProjectPoint(const Camera25D *cam, WorldVector3 vec) {
  f32 scale = cam->focalLength / vec.z;

  return (Vector2){
      .x = cam->centerX + vec.x * scale,
      .y = cam->horizonY + (cam->cameraHeight - vec.y) * scale,
  };
}

static void DrawQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color) {
  DrawTriangle(a, b, c, color);
  DrawTriangle(a, c, d, color);
}

static void InitGame(GameState *game) {
  memset(game, 0, sizeof(*game));

  game->roadScroll = 0.0f;
  game->playerSpeed = 0.0f;
  game->roadBottomY = 720.0f;
  game->leftRoadLineX = -3.75f;
  game->centerRoadLineX = 3.75f;
  game->rightRoadLineX = 13.5f;

  game->carCount = 1;
  game->cars[0] = (TrafficCar){4.0f, 45.0f, 1.8f, 1.3f, 4.0f, RED};
  // game->cars[1] = (TrafficCar){3.0f, 85.0f, 1.8f, 1.3f, 4.0f, BLUE};
  // game->cars[2] = (TrafficCar){3.0f, 130.0f, 1.8f, 1.3f, 4.0f, GREEN};
}

static void InitRenderer(Renderer *renderer, u32 width, u32 height) {
  memset(renderer, 0, sizeof(*renderer));

  renderer->screenWidth = width;
  renderer->screenHeight = height;
  renderer->camera = (Camera25D){
      .centerX = 309.0f,
      .horizonY = 244.0f,
      .focalLength = 204.0f,
      .cameraHeight = 3.13f,
  };
  renderer->target = LoadRenderTexture((i32)width, (i32)height);
  SetTextureFilter(renderer->target.texture, TEXTURE_FILTER_BILINEAR);

  renderer->background = LoadTexture("resources/background.png");
  ASSERT(IsTextureValid(renderer->background), "failed to load background");

  renderer->trafficCar = LoadTexture("resources/car.png");
  ASSERT(IsTextureValid(renderer->trafficCar), "failed to load traffic car");
}

static void ShutdownRenderer(Renderer *renderer) {
  UnloadTexture(renderer->trafficCar);
  UnloadTexture(renderer->background);
  UnloadRenderTexture(renderer->target);
  memset(renderer, 0, sizeof(*renderer));
}

static void UpdateGame(GameState *game, f32 dt) {
  game->roadScroll -= game->playerSpeed * dt;
  if (game->roadScroll < 0.0f) {
    game->roadScroll += 180.0f;
  }

  for (i32 i = 0; i < game->carCount; i++) {
    game->cars[i].z -= game->playerSpeed * dt;

    if (game->cars[i].z < -4.5f) {
      game->cars[i].z = 150.0f + (f32)i * 35.0f;
    }
  }
}

static void DrawRoad(const Renderer *renderer, const GameState *game) {
  const Camera25D *cam = &renderer->camera;

  // dashed divider
  for (i32 i = 0; i < 40; i++) {
    f32 z0 = 1.0f + fmodf(game->roadScroll + (f32)i * 9.0f, 180.0f);
    f32 z1 = z0 + 4.0f;

    Vector2 a =
        ProjectPoint(cam, (WorldVector3){game->centerRoadLineX, 0.0f, z0});
    Vector2 b =
        ProjectPoint(cam, (WorldVector3){game->centerRoadLineX, 0.0f, z1});
    f32 thickness = Clamp(0.12f * cam->focalLength / z0, 1.0f, 16.0f);

    DrawLineEx(a, b, thickness, WHITE);
  }
}

static void DrawDebugUi(Renderer *renderer, GameState *game) {
  (void)renderer;

  TrafficCar *car = &game->cars[0];

  GuiPanel((Rectangle){12, 64, 276, 112}, "traffic car tuning");

  GuiSliderBar((Rectangle){104, 92, 128, 16}, "position",
               TextFormat("%.1f", car->z), &car->z, 1.1f, 150.0f);
  GuiSliderBar((Rectangle){104, 148, 128, 16}, "speed",
               TextFormat("%.1f", game->playerSpeed), &game->playerSpeed, 0.0f,
               90.0f);

  f32 viewAngle = atan2f(car->x, car->z);
  DrawText(TextFormat("view angle: %.02f", viewAngle), 104, 196, 16, BLACK);
}

static void DrawDebug(Renderer *renderer, GameState *game) {

  const Camera25D *cam = &renderer->camera;
  const Texture2D texture = renderer->trafficCar;

  const f32 frameWidth = (f32)texture.width / 2.0f;
  const f32 frameHeight = (f32)texture.height / 2.0f;
  const TrafficCar *car = &game->cars[0];

  if (car->z > 1.0f) {

    f32 viewAngle = atan2f(car->x, car->z);

    i32 frameColumn;
    i32 frameRow;

    // ASSERT(viewAngle >= 0.0f, "view angle less than zero");
    if (viewAngle < -0.0f) {
      // Shouldn't happen
      LOG("warn: negative view angle");
      frameColumn = 0;
      frameRow = 1;
    } else if (viewAngle < 0.10f) {
      frameColumn = 0;
      frameRow = 1;
    } else if (viewAngle < 0.30f) {
      frameColumn = 0;
      frameRow = 0;
    } else if (viewAngle < 0.92f) {
      frameColumn = 1;
      frameRow = 1;
    } else {
      frameColumn = 1;
      frameRow = 0;
    }

    Rectangle source = {
        .x = (f32)frameColumn * frameWidth,
        .y = (f32)frameRow * frameHeight,
        .width = frameWidth,
        .height = frameHeight,
    };

    Vector2 groundPosition =
        ProjectPoint(cam, (WorldVector3){car->x, 0.0f, car->z});

    f32 scale = cam->focalLength / car->z;
    f32 drawWidth = car->width * scale * trafficCarScale;
    f32 drawHeight = car->height * scale * trafficCarScale;

    Rectangle destination = {
        .x = groundPosition.x,
        .y = groundPosition.y,
        .width = drawWidth,
        .height = drawHeight,
    };

    Vector2 origin = {
        .x = drawWidth * 0.5f,
        .y = drawHeight,
    };

    DrawTexturePro(texture, source, destination, origin, 0.0f, WHITE);
  }
}

static void DrawGame(Renderer *renderer, GameState *game) {

  BeginTextureMode(renderer->target);
  {
    DrawTexture(renderer->background, 0, 0, WHITE);
    DrawRoad(renderer, game);

    DrawDebug(renderer, game);

    DrawText("Passing slower cars in the right lane", 20, 20, 20, RAYWHITE);
  }
  EndTextureMode();

  BeginDrawing();
  {
    ClearBackground(BLACK);

    DrawTexturePro(renderer->target.texture,
                   (Rectangle){0, 0, (f32)renderer->target.texture.width,
                               -(f32)renderer->target.texture.height},
                   (Rectangle){0, 0, (f32)renderer->screenWidth,
                               (f32)renderer->screenHeight},
                   (Vector2){0, 0}, 0.0f, WHITE);
    DrawDebugUi(renderer, game);
  }
  EndDrawing();
}

////////////////////////////////////////////////////////////////////////////////
/// Application
////////////////////////////////////////////////////////////////////////////////

static void UpdateDrawFrame(void);

i32 main(void) {
#if !defined(_DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  InitWindow(screenWidth, screenHeight, "raylib gamejam template");

  InitGame(&app.game);
  InitRenderer(&app.renderer, screenWidth, screenHeight);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    UpdateDrawFrame();
  }
#endif

  ShutdownRenderer(&app.renderer);

  CloseWindow();
  return 0;
}

void UpdateDrawFrame(void) {
  f32 dt = GetFrameTime();

  UpdateGame(&app.game, dt);
  DrawGame(&app.renderer, &app.game);

  app.frameCounter++;
}
