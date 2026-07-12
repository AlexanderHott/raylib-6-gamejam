#include "raylib.h"
#include "raymath.h"
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

static const u32 trafficCarFrameColumns = 4;
static const u32 trafficCarFrameCount = 13;
static const Vector2 trafficCarFrameAnchors[] = {
    {575.0f, 422.0f}, {558.5f, 420.0f}, {536.0f, 415.0f},
    {521.0f, 414.0f}, {509.0f, 411.0f}, {482.5f, 399.0f},
    {470.0f, 393.0f}, {460.0f, 392.0f}, {440.0f, 391.0f},
    {425.5f, 387.0f}, {384.5f, 381.0f}, {363.5f, 375.0f},
    {346.5f, 375.0f},
};

////////////////////////////////////////////////////////////////////////////////
/// Game
////////////////////////////////////////////////////////////////////////////////

#define TRAFFIC_CARS_MAX 16
#define HEX_CHECKPOINTS_MAX 32
#define HEX_TRAIL_POINTS_MAX 1024

typedef enum HexGestureState {
  HEX_GESTURE_IDLE,
  HEX_GESTURE_DRAWING,
  HEX_GESTURE_SUCCESS,
  HEX_GESTURE_FAILURE,
} HexGestureState;

typedef enum GestureAnchorState {
  GESTURE_ANCHOR_OFF,
  GESTURE_ANCHOR_ON,
  GESTURE_ANCHOR_NEXT,
} GestureAnchorState;

typedef struct HexShape {
  const char *name;
  Vector2 checkpoints[HEX_CHECKPOINTS_MAX];
  u32 checkpointCount;
} HexShape;

static const HexShape hexShapes[] = {
    {.name = "star",
     .checkpoints = {{0.50f, 0.04f},
                     {0.62f, 0.37f},
                     {0.96f, 0.37f},
                     {0.69f, 0.58f},
                     {0.79f, 0.94f},
                     {0.50f, 0.73f},
                     {0.21f, 0.94f},
                     {0.31f, 0.58f},
                     {0.04f, 0.37f},
                     {0.38f, 0.37f},
                     {0.50f, 0.04f}},
     .checkpointCount = 11},
    {.name = "bolt",
     .checkpoints = {{0.62f, 0.04f},
                     {0.20f, 0.54f},
                     {0.48f, 0.54f},
                     {0.36f, 0.96f},
                     {0.80f, 0.42f},
                     {0.53f, 0.42f},
                     {0.62f, 0.04f}},
     .checkpointCount = 7},
    {.name = "hourglass",
     .checkpoints = {{0.18f, 0.08f},
                     {0.82f, 0.08f},
                     {0.50f, 0.50f},
                     {0.82f, 0.92f},
                     {0.18f, 0.92f},
                     {0.50f, 0.50f},
                     {0.18f, 0.08f}},
     .checkpointCount = 7},
    {.name = "diamond",
     .checkpoints = {{0.50f, 0.04f},
                     {0.94f, 0.50f},
                     {0.50f, 0.96f},
                     {0.06f, 0.50f},
                     {0.50f, 0.04f},
                     {0.50f, 0.96f}},
     .checkpointCount = 6},
    {.name = "check engine",
     .checkpoints =
         {{0.4700f, 0.33015f}, {0.6000f, 0.3300f}, {0.6005f, 0.4125f},
          {0.7130f, 0.4130f},  {0.7130f, 0.5070f}, {0.7840f, 0.5070f},
          {0.7844f, 0.4420f},  {0.8544f, 0.4420f}, {0.8544f, 0.7000f},
          {0.7810f, 0.7000f},  {0.7810f, 0.6346f}, {0.7100f, 0.6340f},
          {0.7100f, 0.7497f},  {0.4760f, 0.7500f}, {0.3490f, 0.6690f},
          {0.2160f, 0.6690f},  {0.2140f, 0.5440f}, {0.1500f, 0.5437f},
          {0.1360f, 0.6740f},  {0.1350f, 0.3930f}, {0.1506f, 0.5100f},
          {0.2130f, 0.5100f},  {0.2160f, 0.3870f}, {0.2960f, 0.3854f},
          {0.2960f, 0.3330f},  {0.4323f, 0.3300f}, {0.4327f, 0.2650f},
          {0.3210f, 0.2433f},  {0.5840f, 0.2430f}, {0.4703f, 0.2652f},
          {0.4699f, 0.3301f}},
     .checkpointCount = 31},
};

#define HEX_SHAPE_COUNT (sizeof(hexShapes) / sizeof(hexShapes[0]))

typedef struct HexGesture {
  const HexShape *shape;
  Rectangle bounds;
  Vector2 trail[HEX_TRAIL_POINTS_MAX];
  u32 trailCount;
  u32 nextCheckpoint;
  HexGestureState state;
  f32 resultAge;
  f32 anchorRevealAge;
} HexGesture;

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

  HexGesture gesture;
  u32 shapeIndex;

  f32 roadScroll;
  f32 playerSpeed;
  f32 roadBottomY;
  f32 leftRoadLineX;
  f32 centerRoadLineX;
  f32 rightRoadLineX;

  f32 trafficCarScale;
  f32 trafficCarOffsetX;
  f32 trafficCarOffsetY;
} GameState;

typedef struct Renderer {
  Camera25D camera;
  RenderTexture2D target;

  Texture2D background;
  Texture2D trafficCar;
  Texture2D carInterior;
  Texture2D wizardHand;
  Texture2D gestureAnchor;

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

static Vector2 GetCheckpointPosition(const HexGesture *gesture, u32 index) {
  Vector2 point = gesture->shape->checkpoints[index];
  return (Vector2){
      .x = gesture->bounds.x + point.x * gesture->bounds.width,
      .y = gesture->bounds.y + point.y * gesture->bounds.height,
  };
}

static f32 DistanceToSegment(Vector2 point, Vector2 start, Vector2 end) {
  Vector2 segment = Vector2Subtract(end, start);
  f32 lengthSquared = Vector2LengthSqr(segment);
  if (lengthSquared == 0.0f) {
    return Vector2Distance(point, start);
  }

  f32 t =
      Vector2DotProduct(Vector2Subtract(point, start), segment) / lengthSquared;
  t = Clamp(t, 0.0f, 1.0f);
  Vector2 nearest = Vector2Add(start, Vector2Scale(segment, t));
  return Vector2Distance(point, nearest);
}

static void AdvanceHexCheckpoints(HexGesture *gesture, Vector2 start,
                                  Vector2 end) {
  const f32 checkpointRadius = 24.0f;

  while (gesture->nextCheckpoint < gesture->shape->checkpointCount) {
    Vector2 checkpoint =
        GetCheckpointPosition(gesture, gesture->nextCheckpoint);
    if (DistanceToSegment(checkpoint, start, end) > checkpointRadius) {
      break;
    }
    gesture->nextCheckpoint++;
  }
}

static void BeginHexGesture(HexGesture *gesture, Vector2 cursor) {
  gesture->trail[0] = cursor;
  gesture->trailCount = 1;
  gesture->nextCheckpoint = 0;
  gesture->state = HEX_GESTURE_DRAWING;
  gesture->resultAge = 0.0f;
  AdvanceHexCheckpoints(gesture, cursor, cursor);
}

static bool UpdateHexGesture(HexGesture *gesture, Vector2 cursor, f32 dt) {
  const f32 sampleDistance = 4.0f;
  const f32 resultDuration = 0.75f;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    BeginHexGesture(gesture, cursor);
  }

  if (gesture->state == HEX_GESTURE_DRAWING &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 previous = gesture->trail[gesture->trailCount - 1];
    if (Vector2Distance(previous, cursor) >= sampleDistance &&
        gesture->trailCount < HEX_TRAIL_POINTS_MAX) {
      gesture->trail[gesture->trailCount++] = cursor;
      AdvanceHexCheckpoints(gesture, previous, cursor);
    }
  }

  if (gesture->state == HEX_GESTURE_DRAWING &&
      IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    gesture->state = gesture->nextCheckpoint == gesture->shape->checkpointCount
                         ? HEX_GESTURE_SUCCESS
                         : HEX_GESTURE_FAILURE;
    gesture->resultAge = 0.0f;
  }

  if (gesture->state == HEX_GESTURE_SUCCESS ||
      gesture->state == HEX_GESTURE_FAILURE) {
    gesture->resultAge += dt;
    if (gesture->resultAge >= resultDuration) {
      bool completed = gesture->state == HEX_GESTURE_SUCCESS;
      gesture->state = HEX_GESTURE_IDLE;
      gesture->trailCount = 0;
      gesture->nextCheckpoint = 0;
      return completed;
    }
  }

  return false;
}

static void InitGame(GameState *game) {
  memset(game, 0, sizeof(*game));

  game->roadScroll = 0.0f;
  game->playerSpeed = 0.0f;
  game->roadBottomY = 720.0f;
  game->leftRoadLineX = -3.75f;
  game->centerRoadLineX = 3.75f;
  game->rightRoadLineX = 13.5f;
  game->trafficCarScale = 10.0f;
  game->trafficCarOffsetX = 1.0f;
  game->trafficCarOffsetY = 0.0f;

  game->carCount = 1;
  game->cars[0] = (TrafficCar){4.0f, 45.0f, 1.8f, 1.3f, 4.0f, RED};
  // game->cars[1] = (TrafficCar){3.0f, 85.0f, 1.8f, 1.3f, 4.0f, BLUE};
  // game->cars[2] = (TrafficCar){3.0f, 130.0f, 1.8f, 1.3f, 4.0f, GREEN};

  game->shapeIndex = 0;
  game->gesture.shape = &hexShapes[game->shapeIndex];
  game->gesture.bounds = (Rectangle){0.0f, 0.0f, 720.0f, 720.0f};
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
  ASSERT(IsTextureValid(renderer->background),
         "failed to load background texture");

  renderer->trafficCar = LoadTexture("resources/traffic-car.png");
  ASSERT(IsTextureValid(renderer->trafficCar),
         "failed to load traffic car texture");

  renderer->carInterior = LoadTexture("resources/car-interior.png");
  ASSERT(IsTextureValid(renderer->carInterior),
         "failed to load car interior texture");

  renderer->wizardHand = LoadTexture("resources/wizard-hand.png");
  ASSERT(IsTextureValid(renderer->wizardHand),
         "failed to load wizard hand texture");

  renderer->gestureAnchor = LoadTexture("resources/gesture-anchor.png");
  ASSERT(IsTextureValid(renderer->gestureAnchor),
         "failed to load gesture anchor texture");
}

static void ShutdownRenderer(Renderer *renderer) {
  UnloadTexture(renderer->gestureAnchor);
  UnloadTexture(renderer->wizardHand);
  UnloadTexture(renderer->carInterior);
  UnloadTexture(renderer->trafficCar);
  UnloadTexture(renderer->background);
  UnloadRenderTexture(renderer->target);
  memset(renderer, 0, sizeof(*renderer));
}

static void UpdateGame(GameState *game, f32 dt) {
  game->gesture.anchorRevealAge += dt;
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

static const char *GetHexGestureStateName(HexGestureState state) {
  switch (state) {
  case HEX_GESTURE_IDLE:
    return "idle";
  case HEX_GESTURE_DRAWING:
    return "drawing";
  case HEX_GESTURE_SUCCESS:
    return "success";
  case HEX_GESTURE_FAILURE:
    return "failure";
  }
  return "unknown";
}

static GestureAnchorState GetGestureAnchorState(const HexGesture *gesture,
                                                u32 index) {
  if (index < gesture->nextCheckpoint) {
    return GESTURE_ANCHOR_ON;
  }
  if (index == gesture->nextCheckpoint) {
    return GESTURE_ANCHOR_NEXT;
  }
  return GESTURE_ANCHOR_OFF;
}

static void DrawGestureAnchor(Texture2D texture, Vector2 position,
                              GestureAnchorState state, f32 revealProgress) {
  const f32 frameWidth = (f32)texture.width / 3.0f;
  const f32 frameHeight = (f32)texture.height;
  f32 eased = 1.0f - powf(1.0f - revealProgress, 3.0f);
  f32 scale = 0.65f + 0.35f * eased;
  Rectangle source = {(f32)state * frameWidth, 0.0f, frameWidth, frameHeight};
  Rectangle destination = {position.x, position.y, frameWidth * scale,
                           frameHeight * scale};
  Vector2 origin = {destination.width * 0.5f, destination.height * 0.5f};

  DrawTexturePro(texture, source, destination, origin, 0.0f,
                 Fade(WHITE, eased));
}

static void DrawHexGesture(const Renderer *renderer,
                           const HexGesture *gesture) {
  const f32 anchorStagger = 0.200f;
  const f32 anchorRevealDuration = 0.500f;
  Color trailColor = (Color){190, 100, 255, 255};
  if (gesture->state == HEX_GESTURE_SUCCESS) {
    trailColor = (Color){80, 255, 180, 255};
  } else if (gesture->state == HEX_GESTURE_FAILURE) {
    trailColor = (Color){255, 60, 80, 255};
  }

  for (u32 i = 0; i < gesture->shape->checkpointCount; i++) {
    f32 delay = (f32)i * anchorStagger;
    f32 progress =
        Clamp((gesture->anchorRevealAge - delay) / anchorRevealDuration, 0.0f,
              1.0f);
    if (progress > 0.0f) {
      DrawGestureAnchor(renderer->gestureAnchor,
                        GetCheckpointPosition(gesture, i),
                        GetGestureAnchorState(gesture, i), progress);
    }
  }

  for (u32 i = 1; i < gesture->trailCount; i++) {
    DrawLineEx(gesture->trail[i - 1], gesture->trail[i], 11.0f,
               Fade(trailColor, 0.20f));
    DrawLineEx(gesture->trail[i - 1], gesture->trail[i], 4.0f, trailColor);
  }
}

static void DrawHexGestureDebugUi(const HexGesture *gesture) {
  DrawRectangle(12, 596, 250, 112, Fade(BLACK, 0.72f));
  DrawRectangleLines(12, 596, 250, 112, Fade(RAYWHITE, 0.55f));
  DrawText("GESTURE DEBUG", 24, 606, 18, RAYWHITE);
  DrawText(TextFormat("shape: %s", gesture->shape->name), 24, 630, 16,
           LIGHTGRAY);
  DrawText(TextFormat("state: %s", GetHexGestureStateName(gesture->state)), 24,
           650, 16, LIGHTGRAY);
  DrawText(TextFormat("checkpoint: %u / %u", gesture->nextCheckpoint,
                      gesture->shape->checkpointCount),
           24, 670, 16, LIGHTGRAY);
  DrawText(TextFormat("trail samples: %u", gesture->trailCount), 24, 690, 16,
           LIGHTGRAY);
}

static void DrawTrafficCars(Renderer *renderer, GameState *game) {

  const Camera25D *cam = &renderer->camera;
  const Texture2D texture = renderer->trafficCar;

  const f32 frameWidth = (f32)texture.width / (f32)trafficCarFrameColumns;
  const f32 frameHeight = frameWidth;
  const TrafficCar *car = &game->cars[0];

  if (car->z > 1.0f) {

    f32 viewAngle = atan2f(car->x, car->z);

    // The sprite sheet only contains views from the car's right side.
    if (viewAngle < 0.0f) {
      return;
    }

    f32 frameProgress = Clamp(viewAngle / (PI * 0.5f), 0.0f, 1.0f);
    u32 frameIndex = (trafficCarFrameCount - 1) -
                     (u32)roundf(frameProgress *
                                 (f32)(trafficCarFrameCount - 1));
    u32 frameColumn = frameIndex % trafficCarFrameColumns;
    u32 frameRow = frameIndex / trafficCarFrameColumns;

    Rectangle source = {
        .x = (f32)frameColumn * frameWidth,
        .y = (f32)frameRow * frameHeight,
        .width = frameWidth,
        .height = frameHeight,
    };

    Vector2 groundPosition =
        ProjectPoint(cam, (WorldVector3){car->x, 0.0f, car->z});

    f32 scale = cam->focalLength / car->z;
    f32 drawWidth = car->width * scale * game->trafficCarScale;
    f32 drawHeight = car->height * scale * game->trafficCarScale;

    Rectangle destination = {
        .x = groundPosition.x + game->trafficCarOffsetX * scale,
        .y = groundPosition.y + game->trafficCarOffsetY * scale,
        .width = drawWidth,
        .height = drawHeight,
    };

    Vector2 frameAnchor = trafficCarFrameAnchors[frameIndex];
    Vector2 origin = {
        .x = drawWidth * frameAnchor.x / frameWidth,
        .y = drawHeight * frameAnchor.y / frameHeight,
    };

    DrawTexturePro(texture, source, destination, origin, 0.0f, WHITE);
  }
}

static void DrawWizardHand(const Renderer *renderer,
                           const HexGesture *gesture) {
  if (gesture->state != HEX_GESTURE_DRAWING) {
    return;
  }

  Vector2 position =
      Vector2Subtract(GetMousePosition(), (Vector2){93.0f, 97.0f});
  DrawTextureV(renderer->wizardHand, position, WHITE);
}

static void DrawGame(Renderer *renderer, GameState *game) {

  BeginTextureMode(renderer->target);
  {
    DrawTexture(renderer->background, 0, 0, WHITE);
    DrawRoad(renderer, game);

    DrawTrafficCars(renderer, game);

    DrawTexture(renderer->carInterior, 0, 0, WHITE);
    DrawWizardHand(renderer, &game->gesture);
    DrawHexGesture(renderer, &game->gesture);

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
    DrawHexGestureDebugUi(&game->gesture);
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

  if (UpdateHexGesture(&app.game.gesture, GetMousePosition(), dt)) {
    app.game.shapeIndex = (app.game.shapeIndex + 1) % HEX_SHAPE_COUNT;
    app.game.gesture.shape = &hexShapes[app.game.shapeIndex];
    app.game.gesture.anchorRevealAge = 0.0f;
  }
  UpdateGame(&app.game, dt);
  DrawGame(&app.renderer, &app.game);

  app.frameCounter++;
}
