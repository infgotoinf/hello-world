#include "include/game.h"

#include <stdlib.h> // For NULL
#include <math.h>
#include <stdbool.h>

#include "raylib.h"
#include "raygui.h"
#include "rlgl.h"
#include "raymath.h"

#include "include/global_vars.h"



// Used code from:
// "3d camera fps" example: https://www.raylib.com/examples/core/loader.html?name=core_3d_camera_fps
// Zlib license



// Movement constants
#define GRAVITY         46.0f
#define MAX_SPEED        7.0f
#define CROUCH_SPEED     2.5f
#define JUMP_FORCE      12.0f
#define MAX_ACCEL       50.0f
// Grounded drag
#define FRICTION        0.86f
// Increasing air drag, increases strafing speed
#define AIR_DRAG        0.98f
// Responsiveness for turning movement direction to looked direction
#define CONTROL         15.0f
#define CROUCH_HEIGHT    0.7f
#define STAND_HEIGHT    1.95f
#define CAMERA_CROUCH_HEIGHT  0.5f
#define CAMERA_HEIGHT        1.75f
#define FOV             80.0f

#define DEFAULT_BODY_SIZE 0.25f
#define NUM_OF_BODY_COLLISION_VERTECIES 8.f
// Using radians instead of degrees
// 360 degrees = 2 * PI radians
// This is because cos() and sin() expect angles in radians
#define COLLISION_ANGLE_DIFFERENCE 2 * PI / NUM_OF_BODY_COLLISION_VERTECIES

#define NORMALIZE_INPUT  0

#define MAX_ENEMY_COUNT    50
#define MAX_MAP_MESH_COUNT 50



typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    float size;
    int hp;
    bool isGrounded;
} Body;


typedef struct {
    Body body;
    Camera camera;
} Player;


typedef enum {
    MEELE
   ,GUN
   ,EnemyTypeSize
} EnemyType;

typedef enum {
    IDLE
   ,CHASING
   ,ATTACKING
   ,DYING
   ,EnemyStatusSize
} EnemyStatus;

typedef enum {
    STAND
   ,WALK
   ,ATTACK
   ,PAIN
   ,DIE
   ,AnimationTypeSize
} AnimationType;

typedef enum {
    FRONT
   ,RIGHT
   ,LEFT
   ,BACK
   ,SpriteTypeSize
} SpriteDirection;

typedef struct {
    Body body;
    EnemyType type;
    EnemyStatus status;
    void *data;
} Enemy;

typedef struct {
    Enemy *enemy;
    int enemyCount;
} Enemies;


typedef struct {
    Mesh *meshes;
    int *materialIdxes;
    int meshCount;
} Map;



static Vector2 sensitivity = { 0.001f, 0.001f };

static Player player = { 0 };
static Vector2 lookRotation = { 0 };
static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;
static Vector2 lean = { 0 };

static bool debugMode = true;
static bool prevFrameCrouchHold = false;

static Enemies enemies;
static Map map;
static BoundingBox finish;
static RenderTexture2D target;

static Texture2D enemySprites[EnemyTypeSize][AnimationTypeSize][SpriteTypeSize];

static Model level;




static void DrawLevel(Camera camera);
static void UpdateCameraFPS(Camera *camera);
static void UpdateBody(Body *player, float rot, char side, char forward, bool jumpPressed, bool *crouchHold);
static void UpdateEnemies();



static Vector3 CenterMeshPos(Mesh mesh) {
    BoundingBox box = GetMeshBoundingBox(mesh);
    return (Vector3){ (box.min.x + box.max.x) * 0.5f, box.min.y, (box.min.z + box.max.z) * 0.5f };
}



bool LoadGame(void)
{
    static bool firstLoad = true;

    if (firstLoad)
    {
        if (debugMode) SetTraceLogLevel(LOG_ALL);

        const char* pathEnemy = "assets/art/sprites/entities/";
        for (EnemyType type = 0; type < EnemyTypeSize; ++type)
        {
            char pathEnemyType[256];
            int pathLength = TextCopy(pathEnemyType, pathEnemy);

            switch (type) {
            case MEELE:
                TextAppend(pathEnemyType, "self/", &pathLength);
                break;
            case GUN:
                TextAppend(pathEnemyType, "gun/", &pathLength);
                break;
            default:
                TraceLog(LOG_ERROR, "LoadGame: This EnemyType does not exist!");
            }
            for (AnimationType status = 0; status < AnimationTypeSize; ++status)
            {
                char pathEnemyAnimation[256];
                pathLength = TextCopy(pathEnemyAnimation, pathEnemyType);

                switch (status) {
                case STAND:
                    TextAppend(pathEnemyAnimation, "stand/", &pathLength);
                    break;
                case WALK:
                    TextAppend(pathEnemyAnimation, "walk/", &pathLength);
                    break;
                case ATTACK:
                    TextAppend(pathEnemyAnimation, "attack/", &pathLength);
                    break;
                case PAIN:
                    TextAppend(pathEnemyAnimation, "pain/", &pathLength);
                    break;
                case DIE:
                    TextAppend(pathEnemyAnimation, "die/", &pathLength);
                    break;
                default:
                    TraceLog(LOG_ERROR, "LoadGame: This EnemyStatus does not exist!");
                }
                for (SpriteDirection sprite = 0; sprite < SpriteTypeSize; ++sprite)
                {
                    char pathEnemySprite[256];
                    pathLength = TextCopy(pathEnemySprite, pathEnemyAnimation);

                    switch (sprite) {
                    case FRONT:
                        TextAppend(pathEnemySprite, "f.png", &pathLength);
                        break;
                    case RIGHT:
                        TextAppend(pathEnemySprite, "r.png", &pathLength);
                        break;
                    case LEFT:
                        TextAppend(pathEnemySprite, "l.png", &pathLength);
                        break;
                    case BACK:
                        TextAppend(pathEnemySprite, "b.png", &pathLength);
                        break;
                    default:
                        TraceLog(LOG_ERROR, "LoadGame: This EnemyStatus does not exist!");
                    }

                    enemySprites[type][status][sprite] = LoadTexture(pathEnemySprite);
                }
            }
        }

        level = LoadModel("assets/maps/map.glb");
        if (!IsModelValid(level)) {
            TraceLog(LOG_ERROR, "Couldn't load Model!");
            return false;
        }

        target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
        firstLoad = false;

        enemies.enemy     = MemAlloc(sizeof(Enemy) * MAX_ENEMY_COUNT);
        map.meshes        = MemAlloc(sizeof(Mesh)  * MAX_MAP_MESH_COUNT);
        map.materialIdxes = MemAlloc(sizeof(int)   * MAX_MAP_MESH_COUNT);
    }


    // Parsing meshes, determening their type, creating map and enemies
    //----------------------------------------------------------------------------------
    if (debugMode)
    {
        TraceLog(LOG_DEBUG, "%d %d", level.materialCount, level.meshCount);
        for (int i = 0; i < level.materialCount; ++i)
        {
            TraceLog(LOG_DEBUG, "%d %d %d %d", level.materials[i].maps[0].color.r
                                             , level.materials[i].maps[0].color.g
                                             , level.materials[i].maps[0].color.b
                                             , level.materials[i].maps[0].color.a);

            TraceLog(LOG_DEBUG, "");
        }
    }

    const Color spawnColor      = {   0,   0, 255, 255 };
    const Color finishColor     = {   0, 255,   0, 255 };
    const Color meeleEnemyColor = { 255,   0,   0, 255 };
    const Color gunEnemyColor   = { 255,   0, 126, 255 };

    Vector3 spawnPos;
    int enemyCounter = 0;
    int mapCounter = 0;
    for (int i = 0; i < level.meshCount; ++i)
    {
        int materialIdxes = level.meshMaterial[i];
        Color meshMaterialColor = level.materials[materialIdxes].maps[0].color;

        if      (ColorIsEqual(meshMaterialColor, spawnColor)) spawnPos = CenterMeshPos(level.meshes[i]);
        else if (ColorIsEqual(meshMaterialColor, finishColor)) finish  = GetMeshBoundingBox(level.meshes[i]);
        else if (ColorIsEqual(meshMaterialColor, meeleEnemyColor))
        {
            enemies.enemy[enemyCounter].type = MEELE;
            enemies.enemy[enemyCounter].status = IDLE;

            Body body = {
                .position = CenterMeshPos(level.meshes[i]),
                .velocity = Vector3Zero(), .dir = Vector3Zero(), .isGrounded = true,

                .size = DEFAULT_BODY_SIZE,
                .hp = 100
            };
            enemies.enemy[enemyCounter++].body = body;
        }
        else if (ColorIsEqual(meshMaterialColor, gunEnemyColor))
        {
            enemies.enemy[enemyCounter].type = GUN;
            enemies.enemy[enemyCounter].status = IDLE;

            Body body = {
                .position = CenterMeshPos(level.meshes[i]),
                .velocity = Vector3Zero(), .dir = Vector3Zero(), .isGrounded = true,

                .size = DEFAULT_BODY_SIZE,
                .hp = 100
            };
            enemies.enemy[enemyCounter++].body = body;
        }
        else
        {
            map.materialIdxes[mapCounter] = materialIdxes;
            map.meshes[mapCounter++] = level.meshes[i];
        }
    }

    enemies.enemyCount = enemyCounter;
    map.meshCount = mapCounter;

    Body playerBody = {
        .position = spawnPos,
        .velocity = Vector3Zero(), .dir = Vector3Zero(),

        .size = DEFAULT_BODY_SIZE,
        .hp = 100
    };
    player.body = playerBody;

    player.camera.fovy = FOV;
    player.camera.projection = CAMERA_PERSPECTIVE;
    player.camera.position = (Vector3) {
        player.body.position.x
       ,CAMERA_HEIGHT
       ,player.body.position.z
    };

    UpdateCameraFPS(&player.camera);

    return true;
}


void UnloadGame(void)
{
    for (EnemyType type = 0; type < EnemyTypeSize; ++type)
        for (AnimationType status = 0; status < AnimationTypeSize; ++status)
            for (SpriteDirection sprite = 0; sprite < SpriteTypeSize; ++sprite)
                UnloadTexture(enemySprites[type][status][sprite]);

    UnloadModel(level);
}


PauseCode DrawPause(void) {
    BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height },
                (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);
        DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){ 0, 0, 0, 100 });

        static const int BUTTON_WIDTH = 200;
        static const int BUTTON_HEIGHT = GUI_TEXT_SIZE + GUI_TEXT_MARGIN;
        static const int BUTTON_MARGIN = GUI_TEXT_MARGIN;
        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5
                                 , WINDOW_HEIGHT * 0.5 - BUTTON_HEIGHT - BUTTON_MARGIN
                                 , BUTTON_WIDTH, BUTTON_HEIGHT }, "continue"))
            return CONTINUE;

        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5
                                 , WINDOW_HEIGHT * 0.5
                                 , BUTTON_WIDTH, BUTTON_HEIGHT }, "restart"))
            return RESTART;

        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5
                                 , WINDOW_HEIGHT * 0.5 + BUTTON_HEIGHT + BUTTON_MARGIN
                                 , BUTTON_WIDTH, BUTTON_HEIGHT }, "main menu"))
            return MAIN_MENU;

        if (GuiButton((Rectangle){ (WINDOW_WIDTH - BUTTON_WIDTH) * 0.5
                                 , WINDOW_HEIGHT * 0.5 + (BUTTON_HEIGHT + BUTTON_MARGIN) * 2
                                 , BUTTON_WIDTH, BUTTON_HEIGHT }, "exit to desktop"))
            return EXIT_TO_DESKTOP;

    EndDrawing();
    return PAUSE;
}



void DrawGame(void)
{
    static bool isPaused = false;
    static bool prevFramePaused = false;

    if (prevFramePaused != isPaused) {
        if (prevFramePaused == true)
        {
            DisableCursor();
        }
        else
        {
            EnableCursor();
        }

        prevFramePaused = isPaused;
    }

    if (IsKeyPressed(KEY_ESCAPE))
        isPaused = !isPaused;

    if (isPaused)
    {
        switch(DrawPause()) {
        case PAUSE:
        default:
            // Keep everything as it is
            break;
        case CONTINUE:
            isPaused = false;
            break;
        case RESTART:
            LoadGame();
            isPaused = false;
            break;
        case MAIN_MENU:
            play = false;
            isPaused = false;
            break;
        case EXIT_TO_DESKTOP:
            doExit = true;
            isPaused = false;
            break;
        }
    }
    else
    {
        if (IsKeyPressed(KEY_GRAVE))
            debugMode = !debugMode;

        static Vector3 cameraPosAdd = { 0 };
        if (IsKeyDown(KEY_DOWN))
        {
            cameraPosAdd.x += 0.1;
        }
        if (IsKeyDown(KEY_UP))
        {
            cameraPosAdd.x -= 0.1;
        }

        // Update
        //----------------------------------------------------------------------------------
        Vector2 mouseDelta = GetMouseDelta();
        lookRotation.x -= mouseDelta.x*sensitivity.x;
        lookRotation.y += mouseDelta.y*sensitivity.y;

        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        bool crouching = IsKeyDown(KEY_LEFT_CONTROL);
        UpdateBody(&player.body, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), &crouching);
        prevFrameCrouchHold = crouching;

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (crouching ? CAMERA_CROUCH_HEIGHT : CAMERA_HEIGHT), 20.0f*delta);
        player.camera.position = (Vector3){
            player.body.position.x + cameraPosAdd.x,
            player.body.position.y + headLerp,
            player.body.position.z,
        };

        if (player.body.isGrounded && ((forward != 0) || (sideway != 0)))
        {
            headTimer += delta*3.0f;
            walkLerp = Lerp(walkLerp, 1.0f, 10.0f*delta);
            player.camera.fovy = Lerp(player.camera.fovy, FOV - 5.0f, 5.0f*delta);
        }
        else
        {
            walkLerp = Lerp(walkLerp, 0.0f, 10.0f*delta);
            player.camera.fovy = Lerp(player.camera.fovy, FOV, 5.0f*delta);
        }

        lean.x = Lerp(lean.x, sideway*0.02f, 10.0f*delta);
        lean.y = Lerp(lean.y, forward*0.015f, 10.0f*delta);

        UpdateCameraFPS(&player.camera);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            BeginMode3D(player.camera);
                DrawLevel(player.camera);
            EndMode3D();

            // Draw info box
            if (debugMode)
            {
                DrawRectangle(5, 5, 330, 75, Fade(SKYBLUE, 0.5f));
                DrawRectangleLines(5, 5, 330, 75, BLUE);

                DrawText("Camera controls:", 15, 15, 10, BLACK);
                DrawText("- Move keys: W, A, S, D, Space, Left-Ctrl", 15, 30, 10, BLACK);
                DrawText("- Look around: mouse", 15, 45, 10, BLACK);
                DrawText(TextFormat("- Velocity Len: (%06.3f)", Vector2Length((Vector2){ player.body.velocity.x, player.body.velocity.z })), 15, 60, 10, BLACK);
                DrawText(TextFormat("Mouse Pos: %03.02f %03.02f", GetMouseDelta().x, GetMouseDelta().y), 100, 100, 10, PURPLE);
                DrawText(TextFormat("Player Pos: %03.02f %03.02f", player.camera.position.x, player.camera.position.y), 100, 120, 10, PURPLE);
                DrawText(TextFormat("Player isGrounded: %d", player.body.isGrounded), 100, 140, 10, PURPLE);
                DrawText(TextFormat("%02.02f %02.02f %02.02f", player.body.dir.x, player.body.dir.y, player.body.dir.z), 100, 160, 10, PURPLE);
                DrawText("` TO TURN OFF DEBUG STUFF", 350, 50, 30, RED);

                static const float radius = 75.0f;
                static const Vector2 center = { WINDOW_WIDTH - 100, 100 };
                for (float i = 0; i < 2 * PI; i += COLLISION_ANGLE_DIFFERENCE)
                {
                    // https://gamemath.com/book/cartesianspace.html#trig_functins
                    // cos(angle) = x, sin(angle) = y
                    DrawLineV(center, (Vector2) { center.x - cos(i) * radius, center.y - sin(i) * radius }, RED);
                }
            }
        EndTextureMode();

        BeginDrawing();
            DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height },
                    (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }
}


//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Update body considering current world state
void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool *crouchHold)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward };

#if defined(NORMALIZE_INPUT)
    // Slow down diagonal movement
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);
#endif

    // Cheking that player can stand up

    // (Sorry for duplicating this for loop i'm cringe
    if (prevFrameCrouchHold == true && *crouchHold == false)
    {
        for (float i = 0; i < 2 * PI; i += COLLISION_ANGLE_DIFFERENCE)
        {
            const Ray heightRay = { (Vector3){ body->position.x + cos(i)*(body->size - 0.001f)
                                             , body->position.y + CROUCH_HEIGHT
                                             , body->position.z + sin(i)*(body->size - 0.001f) }
                                  , (Vector3){ 0
                                             , STAND_HEIGHT - CROUCH_HEIGHT
                                             , 0 } };

            for (int j = 0; j < map.meshCount; ++j)
            {
                const RayCollision crouchCeilingCollision =
                        GetRayCollisionMesh(heightRay, map.meshes[j], level.transform);

                if (crouchCeilingCollision.hit && crouchCeilingCollision.distance < STAND_HEIGHT - CROUCH_HEIGHT)
                {
                    *crouchHold = true;
                    goto CroutchCheckEnd;
                }
            }
        }
    }
CroutchCheckEnd: ;

    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY*delta;

    if (body->isGrounded && jumpPressed && !*crouchHold)
    {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;

        // Sound can be played at this moment
        //SetSoundPitch(fxJump, 1.0f + (GetRandomValue(-100, 100)*0.001));
        //PlaySound(fxJump);
    }

    Vector3 front = (Vector3){ sinf(rot), 0.f, cosf(rot) };
    Vector3 right = (Vector3){ cosf(-rot), 0.f, sinf(-rot) };

    Vector3 desiredDir = (Vector3){ input.x*right.x + input.y*front.x
                                  , 0.0f
                                  , input.x*right.z + input.y*front.z, };
    body->dir = Vector3Lerp(body->dir, desiredDir, CONTROL*delta);

    float decel = (body->isGrounded ? FRICTION : AIR_DRAG);
    Vector3 hvel = (Vector3){ body->velocity.x*decel, 0.0f, body->velocity.z*decel };

    float hvelLength = Vector3Length(hvel); // Magnitude
    if (hvelLength < (MAX_SPEED*0.01f)) hvel = (Vector3){ 0 };

    // This is what creates strafing
    float speed = Vector3DotProduct(hvel, body->dir);

    // Whenever the amount of acceleration to add is clamped by the maximum acceleration constant,
    // a Player can make the speed faster by bringing the direction closer to horizontal velocity angle
    // More info here: https://youtu.be/v3zT3Z5apaM?t=165
    float maxSpeed = (*crouchHold ? CROUCH_SPEED : MAX_SPEED);
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL*delta);
    hvel.x += body->dir.x*accel;
    hvel.z += body->dir.z*accel;

    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    body->position.y += body->velocity.y*delta;
    body->position.x += body->velocity.x*delta;
    body->position.z += body->velocity.z*delta;

    //----------------------------------------------------------------------------------
    // Checking collision
    //----------------------------------------------------------------------------------
    bool skipGroundCollisionCheck = false;
    for (float i = 0; i < 2 * PI; i += COLLISION_ANGLE_DIFFERENCE)
    {
        // Wall collision
        //----------------------------------------------------------------------------------
        // https://gamemath.com/book/cartesianspace.html#trig_functins
        // cos(angle) = x, sin(angle) = y
        const RayCollision *collidingRay = NULL;

        const Ray kneeRay = { (Vector3){ body->position.x
                                       , body->position.y + CROUCH_HEIGHT
                                       , body->position.z }
                            , (Vector3){ cos(i)
                                       , 0
                                       , sin(i) } };

        const Ray headRay = { (Vector3){ body->position.x
                                       , body->position.y + STAND_HEIGHT
                                       , body->position.z }
                            , (Vector3){ cos(i)
                                       , 0
                                       , sin(i) } };

        for (int j = 0; j < map.meshCount; ++j)
        {
            const RayCollision downWallCollision = GetRayCollisionMesh(kneeRay, map.meshes[j], level.transform);
            if (downWallCollision.hit && downWallCollision.distance < body->size)
                collidingRay = &downWallCollision;

            const RayCollision upWallCollision   = GetRayCollisionMesh(headRay, map.meshes[j], level.transform);
            if (!*crouchHold && upWallCollision.hit && upWallCollision.distance < body->size)
                collidingRay = &upWallCollision;

            if (collidingRay != NULL)
            {
                // How much body clipped
                float clip = body->size - collidingRay->distance;

                // We reverse clip, if dot product of body.velocity and wallCollision.normal is positive (if
                // body.vector and wallCollision.normal look in the same direction):
                //
                //     wallCollision.normal . wallCollision.velocity
                //
                // More about dot product: https://gamemath.com/book/vectors.html#dot_product_math
                //
                // We do this, so we don't clip through a wall with a reversed orientation and since we
                // have plane walls and planes have only one orientation, we need this, otherwise:
                // - The side of the wall with our orientation will have a nice collision
                // - And the other one will teleport us to the first side
                //
                // wallCollision.normal when hitting right and wrong wall orientation:
                //
                //                                  >
                //                                /
                //                              /
                //      xxxxxxxxx       ---------
                //      ---------       xxxx/xxxx
                //         / \            /
                //       /    \         /
                //     /       >      /
                //
                if (collidingRay->normal.x * body->velocity.x
                  + collidingRay->normal.z * body->velocity.z > 0) clip *= -1;

                // Vector to add to unclip the body
                const Vector3 unclip = { collidingRay->normal.x * clip
                                       , 0
                                       , collidingRay->normal.z * clip };

                body->position = Vector3Add(body->position, unclip);
                collidingRay = NULL;
            }
        }
        // (Sorry for copypasta)
        for (int j = 0; j < enemies.enemyCount; ++j)
        {
            // Checking that this is not the same entity
            if (!Vector3Equals(body->position, enemies.enemy->body.position))
            {
                Body enemyBody = enemies.enemy[j].body;
                BoundingBox enemyBox = { (Vector3){ enemyBody.position.x - enemyBody.size
                                                  , enemyBody.position.y
                                                  , enemyBody.position.z - enemyBody.size }
                                       , (Vector3){ enemyBody.position.x + enemyBody.size
                                                  , enemyBody.position.y + 100.0f
                                                  , enemyBody.position.z + enemyBody.size } };

                const RayCollision downWallCollision = GetRayCollisionBox(kneeRay, enemyBox);
                if (downWallCollision.hit && downWallCollision.distance < body->size)
                    collidingRay = &downWallCollision;

                const RayCollision upWallCollision   = GetRayCollisionBox(headRay, enemyBox);
                if (!*crouchHold && upWallCollision.hit && upWallCollision.distance < body->size)
                    collidingRay = &upWallCollision;

                if (collidingRay != NULL)
                {
                    // How much body clipped
                    float clip = body->size - collidingRay->distance;

                    if (collidingRay->normal.x * body->velocity.x
                      + collidingRay->normal.z * body->velocity.z > 0) clip *= -1;

                    // Vector to add to unclip the body
                    const Vector3 unclip = { collidingRay->normal.x * clip
                                           , 0
                                           , collidingRay->normal.z * clip };

                    body->position = Vector3Add(body->position, unclip);
                    collidingRay = NULL;
                }
            }
        }


        // Floor and ceiling collision
        //----------------------------------------------------------------------------------
        //
        //   ()  ^  - ceilingCollision
        //  /|\  |
        //  /\   +
        //       |  - floorCollision
        //       V

        if (!skipGroundCollisionCheck)
        {
            const Ray downwardRay = { (Vector3){ body->position.x + cos(i)*(body->size - 0.001f)
                                               , body->position.y
                                               , body->position.z + sin(i)*(body->size - 0.001f) }
                                    , (Vector3){ 0, -1000.f, 0 } };

            const Ray upwardRay   = { (Vector3){ body->position.x + cos(i)*(body->size - 0.001f)
                                               , body->position.y
                                               , body->position.z + sin(i)*(body->size - 0.001f) }
                                    , (Vector3){ 0, 1000.f, 0 } };

            RayCollision closestFloorCollision = { .distance = 10000.f };
            RayCollision closestCeilingCollision = { .distance = 10000.f };

            for (int j = 0; j < map.meshCount; ++j)
            {
                const RayCollision floorCollision   = GetRayCollisionMesh(downwardRay, map.meshes[j], level.transform);
                const RayCollision ceilingCollision = GetRayCollisionMesh(upwardRay  , map.meshes[j], level.transform);

                if (floorCollision.hit && floorCollision.distance < closestFloorCollision.distance)
                    closestFloorCollision = floorCollision;

                if (ceilingCollision.hit && ceilingCollision.distance < closestCeilingCollision.distance)
                    closestCeilingCollision = ceilingCollision;
            }

            if (closestCeilingCollision.hit)
            {
                if (closestCeilingCollision.distance * 1000.f <= 0.5f)
                {
                    body->position.y = closestCeilingCollision.point.y;
                    body->isGrounded = true;
                    body->velocity.y = 0.0f;
                    skipGroundCollisionCheck = true;
                    continue;
                }
                else if (!*crouchHold && closestCeilingCollision.distance * 1000.f <= STAND_HEIGHT)
                {
                    body->position.y = closestCeilingCollision.point.y - STAND_HEIGHT - 0.15f;
                    body->isGrounded = false;
                    body->velocity.y = 0.0f;
                    skipGroundCollisionCheck = true;
                    continue;
                }
            }

            if (closestFloorCollision.hit && closestFloorCollision.distance * 1000.f >= 0.01f)
                 body->isGrounded = false;
            else {
                body->isGrounded = true;
                skipGroundCollisionCheck = true;
            }
        }
    }
}


// Update camera for FPS behaviour
static void UpdateCameraFPS(Camera *camera)
{
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };
    // Left and right
    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    // Clamp view up
    float maxAngleUp = Vector3Angle(up, yaw);
    maxAngleUp -= 0.001f; // Avoid numerical errors
    if ( -(lookRotation.y) > maxAngleUp) { lookRotation.y = -maxAngleUp; }

    // Clamp view down
    float maxAngleDown = Vector3Angle(Vector3Negate(up), yaw);
    maxAngleDown *= -1.0f; // Downwards angle is negative
    maxAngleDown += 0.001f; // Avoid numerical errors
    if ( -(lookRotation.y) < maxAngleDown) { lookRotation.y = -maxAngleDown; }

    // Up and down
    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    // Rotate view vector around right axis
    float pitchAngle = -lookRotation.y - lean.y;
    pitchAngle = Clamp(pitchAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f); // Clamp angle so it doesn't go past straight up or straight down
    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    // Head animation
    // Rotate up direction around forward axis
    float headSin = sinf(headTimer*PI);
    float headCos = cosf(headTimer*PI);
    const float stepRotation = 0.01f;
    camera->up = Vector3RotateByAxisAngle(up, pitch, headSin*stepRotation + lean.x);

    // Camera BOB
    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    Vector3 bobbing = Vector3Scale(right, headSin*bobSide);
    bobbing.y = fabsf(headCos*bobUp);

    camera->position = Vector3Add(camera->position, Vector3Scale(bobbing, walkLerp));
    camera->target = Vector3Add(camera->position, pitch);
}


static void DrawBodyMapCollision(Body body)
{
    for (float i = 0; i < 2 * PI; i += COLLISION_ANGLE_DIFFERENCE)
    {
        // https://gamemath.com/book/cartesianspace.html#trig_functins

        const Vector3 kneeVector = { body.position.x + cos(i) * body.size
                                   , body.position.y + CROUCH_HEIGHT
                                   , body.position.z + sin(i) * body.size };

        const Vector3 headVector = { body.position.x + cos(i) * body.size
                                   , body.position.y + STAND_HEIGHT
                                   , body.position.z + sin(i) * body.size };

        DrawSphereEx(kneeVector, 0.02f, 8, 8, WHITE);
        DrawSphereEx(headVector, 0.02f, 8, 8, WHITE);
    }

    Vector3 bodyKnees = { body.position.x
                        , body.position.y + CROUCH_HEIGHT
                        , body.position.z };

    DrawRay((Ray){ bodyKnees, body.dir }, ORANGE);
    DrawRay((Ray){ bodyKnees, body.velocity }, YELLOW);

    BoundingBox bodyBox = { (Vector3){ body.position.x - body.size
                                     , body.position.y
                                     , body.position.z - body.size }
                          , (Vector3){ body.position.x + body.size
                                     , body.position.y + 100.0f
                                     , body.position.z + body.size } };
    DrawBoundingBox(bodyBox, RED);
}


static void DrawEnemies(Camera camera)
{
    for (int i = 0; i < enemies.enemyCount; ++i)
    {
        Enemy enemy = enemies.enemy[i];
        Texture2D enemySprite = enemySprites[enemy.type][enemy.status][FRONT];
        // if (debugMode && !IsTextureValid(enemySprite))
        //     TraceLog(LOG_ERROR, "DrawEnemies: trying to draw not existing texture %d %d %d", enemy.type, enemy.status, FRONT);
        DrawBillboard(camera, enemySprite, (Vector3){ enemies.enemy->body.position.x
                                                    , enemies.enemy->body.position.y / 2
                                                    , enemies.enemy->body.position.z }
                                         , 2.0f, WHITE);
        if (debugMode)
        {
            DrawBodyMapCollision(enemies.enemy->body);
        }
    }
}


// Draw game level
static void DrawLevel(Camera camera)
{
    rlDisableBackfaceCulling();

    // static int a = 0;
    // if (IsKeyPressed(KEY_Q)) ++a;
    // DrawMesh(level.meshes[a], level.materials[0], level.transform);
    for (int i = 0; i < map.meshCount; ++i)
    {
        const Mesh currentMesh = map.meshes[i];
        DrawMesh(currentMesh, level.materials[map.materialIdxes[i]], level.transform);
    }

    if (debugMode)
    {
        DrawBodyMapCollision(player.body);
    }

    DrawModelWires(level, Vector3Zero(), 1.0f, BLACK);
    rlEnableBackfaceCulling();

    DrawEnemies(camera);
}
