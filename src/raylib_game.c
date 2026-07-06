#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include <assert.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

#define ASSERT(condition, message) assert(condition && message)

#define STATIC_ASSERT_JOIN2(a, b) a##b
#define STATIC_ASSERT_JOIN(a, b) STATIC_ASSERT_JOIN2(a, b)
#define STATIC_ASSERT(condition, message) \
    typedef char STATIC_ASSERT_JOIN(static_assertion_failed__##message##__line_, __LINE__)[(condition) ? 1 : -1]

#define UNUSED(identifier) ((void)identifier)

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef float     f32;
typedef double    f64;

STATIC_ASSERT(sizeof(u8)  == 1, u8_is_not_1_byte);
STATIC_ASSERT(sizeof(u16) == 2, u16_is_not_2_bytes);
STATIC_ASSERT(sizeof(u32) == 4, u32_is_not_4_bytes);
STATIC_ASSERT(sizeof(u64) == 8, u64_is_not_8_bytes);

STATIC_ASSERT(sizeof(i8)  == 1, i8_is_not_1_byte);
STATIC_ASSERT(sizeof(i16) == 2, i16_is_not_2_bytes);
STATIC_ASSERT(sizeof(i32) == 4, i32_is_not_4_bytes);
STATIC_ASSERT(sizeof(i64) == 8, i64_is_not_8_bytes);

STATIC_ASSERT(sizeof(f32) == 4, f32_is_not_4_bytes);
STATIC_ASSERT(sizeof(f64) == 8, f32_is_not_8_bytes);

////////////////////////////////////////////////////////////////////////////////
/// Game
////////////////////////////////////////////////////////////////////////////////

typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

static const u32 screenWidth = 720;
static const u32 screenHeight = 720;

static RenderTexture2D target = { 0 };
static u32 frameCounter = 0;

static void UpdateDrawFrame(void);

i32 main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);
#endif
    InitWindow(screenWidth, screenHeight, "raylib gamejam template");
    
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    UnloadRenderTexture(target);
    
    CloseWindow();
    return 0;
}

void UpdateDrawFrame(void)
{
    frameCounter++;

    BeginTextureMode(target);
    {
        ClearBackground(RAYWHITE);
        
        DrawRectangle(70, 90, 200, 200, BLACK);
        DrawRectangle(70 + 16, 90 + 16, 200 - 32, 200 - 32, RAYWHITE);
        DrawText("raylib", 70 + 200 - MeasureText("raylib", 40) - 32, 90 + 200 - 40 - 24, 40, BLACK);

        DrawText("6.x", 290, 90 - 26, 280, BLACK);
        DrawText("GAMEJAM", 70, 90 + 210, 120, MAROON);

        if ((frameCounter/20)%2) DrawText("are you ready?", 160, 500, 50, BLACK);
        
        DrawRectangleLinesEx((Rectangle){ 0, 0, screenWidth, screenHeight }, 16, BLACK);
    }
    EndTextureMode();
    
    BeginDrawing();
    {
        ClearBackground(RAYWHITE);
        
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (f32)target.texture.width, -(f32)target.texture.height }, 
            (Rectangle){ 0, 0, (f32)target.texture.width, (f32)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

    }
    EndDrawing();
}
