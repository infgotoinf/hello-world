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
#define MAX_SPEED        6.5f
#define CROUCH_SPEED     2.5f
#define JUMP_FORCE      10.0f
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
#define NUM_OF_BODY_COLLISION_VERTECIES 4.0f
// Using radians instead of degrees
// 360 degrees = 2 * PI radians
// This is because cos() and sin() expect angles in radians
#define COLLISION_ANGLE_DIFFERENCE 2 * PI / NUM_OF_BODY_COLLISION_VERTECIES

#define NORMALIZE_INPUT  0

#define MAX_ENEMY_COUNT    50
#define MAX_ITEM_COUNT     50
#define MAX_MAP_MESH_COUNT 50

#define ANIMATION_FPS    16.0



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
   ,GUNNER
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
   ,EnemyAnimationSize
} EnemyAnimation;

typedef enum {
    FRONT
   ,RIGHT
   ,LEFT
   ,BACK
   ,DirectionSize
} Direction;

typedef struct {
    Body body;
    EnemyType type;
    EnemyStatus status;
    float data[3];
} Enemy;

typedef struct {
    Enemy *enemies;
    int count;
} Enemies;


typedef enum {
    SLEDGE_HAMMER
   ,PISTOL
   ,WeaponTypeSize
} WeaponType;

typedef enum {
    MEDKIT = WeaponTypeSize // Offseting cause Item.type can be both WeaponType and ItemType
   ,AMMO
   ,TotalItemTypeSize
} ItemType;

typedef enum {
    W_IDLE
   ,W_ATTACK
   ,WeaponAnimationSize
} WeaponAnimation;


typedef enum {
    LAYING
   ,PICKED_UP
} ItemStatus;

typedef struct {
    BoundingBox hitbox;
    Vector3 pos;
    int type;
    ItemStatus status;
} Item;

typedef struct {
    Item *items;
    int count;
} Items;

typedef struct {
    Texture2D *frames;
    int count;
} AnimationFrames;


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
static Items items;
static Map map;
static BoundingBox finish;
static RenderTexture2D target;
static Item *selectedWeapon;

static Texture2D enemySprites[EnemyTypeSize][EnemyAnimationSize][DirectionSize];
static Texture2D itemSprites[TotalItemTypeSize];
static AnimationFrames weaponAnimations[WeaponTypeSize][WeaponAnimationSize];

static Model level;



static void DrawLevel(Camera camera);
static void UpdateCameraFPS(Camera *camera);
static bool CheckCanUncrouch(Body *body);
static void UpdateBody(Body *player, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
static void UpdateEnemies(void);
static void UpdateItems(void);
static void DrawWeapon(Texture2D currentAnimationFrame);
static Texture2D UpdateWeapon(bool leftMouseClicked);



static Vector3 CenterMeshPos(Mesh mesh)
{
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
            case GUNNER:
                TextAppend(pathEnemyType, "gunner/", &pathLength);
                break;
            default:
                TraceLog(LOG_ERROR, "LoadGame: This EnemyType does not exist!");
                continue;
            }
            for (EnemyAnimation animation = 0; animation < EnemyAnimationSize; ++animation)
            {
                char pathEnemyAnimation[256];
                pathLength = TextCopy(pathEnemyAnimation, pathEnemyType);

                switch (animation) {
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
                    TraceLog(LOG_ERROR, "LoadGame: This EnemyAnimation does not exist!");
                    continue;
                }
                for (Direction dir = 0; dir < DirectionSize; ++dir)
                {
                    char pathDirection[256];
                    pathLength = TextCopy(pathDirection, pathEnemyAnimation);

                    switch (dir) {
                    case FRONT:
                        TextAppend(pathDirection, "f.png", &pathLength);
                        break;
                    case RIGHT:
                        TextAppend(pathDirection, "r.png", &pathLength);
                        break;
                    case LEFT:
                        TextAppend(pathDirection, "l.png", &pathLength);
                        break;
                    case BACK:
                        TextAppend(pathDirection, "b.png", &pathLength);
                        break;
                    default:
                        TraceLog(LOG_ERROR, "LoadGame: This Direction does not exist!");
                        continue;
                    }

                    enemySprites[type][animation][dir] = LoadTexture(pathDirection);
                }
            }
        }

        const char* pathItems = "assets/art/sprites/items/";
        for (int type = 0; type < WeaponTypeSize; ++type)
        {
            char pathType[256];
            int pathLength = TextCopy(pathType, pathItems);

            switch (type) {
            case SLEDGE_HAMMER:
                TextAppend(pathType, "sledge_hammer/", &pathLength);
                break;
            case PISTOL:
                TextAppend(pathType, "pistol/", &pathLength);
                break;
            default:
                TraceLog(LOG_ERROR, "LoadGame: This WeaponType does not exist!");
                continue;
            }

            char pathSprite[256];
            pathLength = TextCopy(pathSprite, pathType);
            TextAppend(pathSprite, "item.png", &pathLength);

            itemSprites[type] = LoadTexture(pathSprite);

            for (WeaponAnimation animation = 0; animation < WeaponAnimationSize; ++animation)
            {
                char pathWeaponAnimation[256];
                pathLength = TextCopy(pathWeaponAnimation, pathType);

                switch (animation) {
                case W_IDLE:
                    TextAppend(pathWeaponAnimation, "idle", &pathLength);
                    break;
                case W_ATTACK:
                    TextAppend(pathWeaponAnimation, "attack", &pathLength);
                    break;
                default:
                    TraceLog(LOG_ERROR, "LoadGame: This WeaponAnimation does not exist!");
                    continue;
                }

                const unsigned frameNumber = GetDirectoryFileCount(pathWeaponAnimation);
                weaponAnimations[type][animation].count  = (int)frameNumber;
                weaponAnimations[type][animation].frames = MemAlloc(sizeof(Texture2D) * frameNumber);

                for (unsigned i = 0; i < frameNumber; ++i)
                {
                    const char *framePath = TextFormat("%s/%04d.png", pathWeaponAnimation, i);
                    weaponAnimations[type][animation].frames[i] = LoadTexture(framePath);
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

        enemies.enemies   = MemAlloc(sizeof(Enemy) * MAX_ENEMY_COUNT);
        items.items       = MemAlloc(sizeof(Item)  * MAX_ITEM_COUNT);
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
        }
    }

    const Color spawnColor        = {   0,   0, 255, 255 };
    const Color finishColor       = {   0, 255,   0, 255 };
    const Color meeleEnemyColor   = { 255,   0,   0, 255 };
    const Color gunEnemyColor     = { 255,   0, 126, 255 };

    const Color sledgeHammerColor = {   0,   0,  12, 255 };

    Vector3 spawnPos;
    int enemyCounter = 0;
    int itemCounter  = 0;
    int mapCounter   = 0;
    for (int i = 0; i < level.meshCount; ++i)
    {
        int materialIdxes = level.meshMaterial[i];
        Color meshMaterialColor = level.materials[materialIdxes].maps[0].color;

        if      (ColorIsEqual(meshMaterialColor, spawnColor)) spawnPos = CenterMeshPos(level.meshes[i]);
        else if (ColorIsEqual(meshMaterialColor, finishColor)) finish  = GetMeshBoundingBox(level.meshes[i]);
        else if (ColorIsEqual(meshMaterialColor, sledgeHammerColor))
        {
            Item item = {
                .type   = SLEDGE_HAMMER,
                .hitbox = GetMeshBoundingBox(level.meshes[i]),
                .pos    = CenterMeshPos(level.meshes[i]),
                .status = LAYING,
            };
            items.items[itemCounter++] = item;
        }
        else if (ColorIsEqual(meshMaterialColor, meeleEnemyColor))
        {
            enemies.enemies[enemyCounter].type   = MEELE;
            enemies.enemies[enemyCounter].status = IDLE;
            enemies.enemies[enemyCounter].data[0]= 0.0f;
            enemies.enemies[enemyCounter].data[1]= 1.0f;
            enemies.enemies[enemyCounter].data[2]= 0.0f;

            Body body = {
                .position = CenterMeshPos(level.meshes[i]),
                .velocity = Vector3Zero(), .dir = Vector3Zero(), .isGrounded = true,

                .size = DEFAULT_BODY_SIZE,
                .hp = 100,
            };
            enemies.enemies[enemyCounter++].body = body;
        }
        else if (ColorIsEqual(meshMaterialColor, gunEnemyColor))
        {
            enemies.enemies[enemyCounter].type   = GUNNER;
            enemies.enemies[enemyCounter].status = IDLE;
            enemies.enemies[enemyCounter].data[0]= 0.0f;
            enemies.enemies[enemyCounter].data[1]= 1.0f;
            enemies.enemies[enemyCounter].data[2]= 0.0f;

            Body body = {
                .position = CenterMeshPos(level.meshes[i]),
                .velocity = Vector3Zero(), .dir = Vector3Zero(), .isGrounded = true,

                .size = DEFAULT_BODY_SIZE,
                .hp = 100,
            };
            enemies.enemies[enemyCounter++].body = body;
        }
        else
        {
            map.materialIdxes[mapCounter] = materialIdxes;
            map.meshes[mapCounter++] = level.meshes[i];
        }
    }

    enemies.count = enemyCounter;
    items.count   = itemCounter;
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

    selectedWeapon = NULL;

    return true;
}


void UnloadGame(void)
{
    for (EnemyType type = 0; type < EnemyTypeSize; ++type)
        for (EnemyAnimation status = 0; status < EnemyAnimationSize; ++status)
            for (Direction sprite = 0; sprite < DirectionSize; ++sprite)
                UnloadTexture(enemySprites[type][status][sprite]);

    UnloadModel(level);
}


PauseCode DrawPause(void)
{
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

        // Update
        //----------------------------------------------------------------------------------
        Vector2 mouseDelta = GetMouseDelta();
        lookRotation.x -= mouseDelta.x*sensitivity.x;
        lookRotation.y += mouseDelta.y*sensitivity.y;

        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        bool crouching = IsKeyDown(KEY_LEFT_CONTROL);
        if (crouching == false && prevFrameCrouchHold == true)
            crouching = !CheckCanUncrouch(&player.body);

        UpdateBody(&player.body, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), crouching);
        prevFrameCrouchHold = crouching;

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (crouching ? CAMERA_CROUCH_HEIGHT : CAMERA_HEIGHT), 20.0f*delta);
        player.camera.position = (Vector3){
            player.body.position.x,
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

        UpdateEnemies();
        UpdateItems();
        const Texture2D currentAnimationFrame = UpdateWeapon(IsMouseButtonPressed(MOUSE_LEFT_BUTTON));

        UpdateCameraFPS(&player.camera);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            BeginMode3D(player.camera);
                DrawLevel(player.camera);
            EndMode3D();

            if (currentAnimationFrame.height != 0)
                DrawWeapon(currentAnimationFrame);

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
                    DrawLineV(center, (Vector2) { center.x - cosf(i) * radius
                                                , center.y - sinf(i) * radius }, RED);
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
static bool CheckCanUncrouch(Body *body)
{
    // Cheking that player can stand up
    // (Sorry for duplicating this for loop i'm cringe
    for (float i = 0; i < 2 * PI; i += COLLISION_ANGLE_DIFFERENCE)
    {
        const Ray heightRay = { (Vector3){ body->position.x + cosf(i)*(body->size - 0.001f)
                                         , body->position.y + CROUCH_HEIGHT
                                         , body->position.z + sinf(i)*(body->size - 0.001f) }
                              , (Vector3){ 0
                                         , STAND_HEIGHT - CROUCH_HEIGHT
                                         , 0 } };

        for (int j = 0; j < map.meshCount; ++j)
        {
            const RayCollision crouchCeilingCollision =
                    GetRayCollisionMesh(heightRay, map.meshes[j], level.transform);

            if (crouchCeilingCollision.hit && crouchCeilingCollision.distance < STAND_HEIGHT - CROUCH_HEIGHT)
            {
                return false;
            }
        }
    }
    return true;
}


// Update body considering current world state
void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward };

#if defined(NORMALIZE_INPUT)
    // Slow down diagonal movement
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);
#endif
    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY*delta;

    if (body->isGrounded && jumpPressed && !crouchHold)
    {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;

        // Sound can be played at this moment
        //SetSoundPitch(fxJump, 1.0f + (GetRandomValue(-100, 100)*0.001));
        //PlaySound(fxJump);
    }

    Vector3 front = (Vector3){ sinf(rot), 0.0f, cosf(rot) };
    Vector3 right = (Vector3){ cosf(-rot), 0.0f, sinf(-rot) };

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
    float maxSpeed = (crouchHold ? CROUCH_SPEED : MAX_SPEED);
    float accel = Clamp(maxSpeed - speed, 0.0f, MAX_ACCEL*delta);
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
    for (float i = COLLISION_ANGLE_DIFFERENCE/2; i < 2 * PI + COLLISION_ANGLE_DIFFERENCE/2; i += COLLISION_ANGLE_DIFFERENCE)
    {
        // Wall collision
        //----------------------------------------------------------------------------------
        // https://gamemath.com/book/cartesianspace.html#trig_functins
        // cos(angle) = x, sin(angle) = y
        const RayCollision *collidingRay = NULL;

        const Ray kneeRay = { (Vector3){ body->position.x
                                       , body->position.y + CROUCH_HEIGHT
                                       , body->position.z }
                            , (Vector3){ cosf(i)
                                       , 0
                                       , sinf(i) } };

        const Ray headRay = { (Vector3){ body->position.x
                                       , body->position.y + STAND_HEIGHT
                                       , body->position.z }
                            , (Vector3){ cosf(i)
                                       , 0
                                       , sinf(i) } };

        for (int j = 0; j < map.meshCount; ++j)
        {
            const RayCollision downWallCollision = GetRayCollisionMesh(kneeRay, map.meshes[j], level.transform);
            if (downWallCollision.hit && downWallCollision.distance < body->size)
                collidingRay = &downWallCollision;

            const RayCollision upWallCollision   = GetRayCollisionMesh(headRay, map.meshes[j], level.transform);
            if (!crouchHold && upWallCollision.hit && upWallCollision.distance < body->size)
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
                // if (collidingRay->normal.x * body->velocity.x
                //   + collidingRay->normal.z * body->velocity.z > 0) clip *= -1;
                if (collidingRay->normal.x * kneeRay.direction.x
                  + collidingRay->normal.z * kneeRay.direction.z > 0) clip *= -1;

                // Vector to add to unclip the body
                const Vector3 unclip = { collidingRay->normal.x * clip
                                       , 0
                                       , collidingRay->normal.z * clip };

                body->position = Vector3Add(body->position, unclip);
                collidingRay = NULL;
            }
        }
        // (Sorry for copypasta)
        for (int j = 0; j < enemies.count; ++j)
        {
            // Checking that this is not the same entity
            if (!Vector3Equals(body->position, enemies.enemies[j].body.position))
            {
                Body enemyBody = enemies.enemies[j].body;
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
                if (!crouchHold && upWallCollision.hit && upWallCollision.distance < body->size)
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
            const Ray downwardRay = { (Vector3){ body->position.x + cosf(i)*(body->size - 0.001f)
                                               , body->position.y
                                               , body->position.z + sinf(i)*(body->size - 0.001f) }
                                    , (Vector3){ 0, -1000.0f, 0 } };

            const Ray upwardRay   = { (Vector3){ body->position.x + cosf(i)*(body->size - 0.001f)
                                               , body->position.y
                                               , body->position.z + sinf(i)*(body->size - 0.001f) }
                                    , (Vector3){ 0, 1000.0f, 0 } };

            RayCollision closestFloorCollision = { .distance = 10000.0f };
            RayCollision closestCeilingCollision = { .distance = 10000.0f };

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
                if (closestCeilingCollision.distance * 1000.0f <= 0.5f)
                {
                    body->position.y = closestCeilingCollision.point.y;
                    body->isGrounded = true;
                    body->velocity.y = 0.0f;
                    skipGroundCollisionCheck = true;
                    continue;
                }
                else if (!crouchHold && closestCeilingCollision.distance * 1000.0f <= STAND_HEIGHT)
                {
                    body->position.y = closestCeilingCollision.point.y - STAND_HEIGHT - 0.15f;
                    body->isGrounded = false;
                    body->velocity.y = 0.0f;
                    skipGroundCollisionCheck = true;
                    continue;
                }
            }

            if (closestFloorCollision.hit && closestFloorCollision.distance * 1000.0f >= 0.01f)
                 body->isGrounded = false;
            else {
                body->isGrounded = true;
                skipGroundCollisionCheck = true;
            }
        }
    }
}


static void UpdateEnemies(void)
{
    for (int i = 0; i < enemies.count; ++i)
    {
        static bool enemyCrouch = false;

        Enemy *currentEnemy = &enemies.enemies[i];
        switch (currentEnemy->status) {
        case IDLE:
            if (currentEnemy->data[0] <= 0)
            {
                currentEnemy->data[0] = (float)GetRandomValue(10, 15) * 0.1f;
                currentEnemy->data[1] = (float)GetRandomValue(-1, 1);
                currentEnemy->data[2] = (float)GetRandomValue(-1, 1);
            }
            else currentEnemy->data[0] -= 1.0f/FPS;
            UpdateBody(&currentEnemy->body, 0.0f, (char)currentEnemy->data[1]
                                                , (char)currentEnemy->data[2], false, enemyCrouch);
            break;
        case CHASING:
            break;
        case ATTACKING:
            break;
        case DYING:
            break;
        default:
            TraceLog(LOG_ERROR, "UpdateEnemies: This EnemyType does not exist!");
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
    for (float i = COLLISION_ANGLE_DIFFERENCE/2; i < 2 * PI + COLLISION_ANGLE_DIFFERENCE/2; i += COLLISION_ANGLE_DIFFERENCE)
    {
        // https://gamemath.com/book/cartesianspace.html#trig_functins

        const Vector3 kneeVector = { body.position.x + cosf(i) * body.size
                                   , body.position.y + CROUCH_HEIGHT
                                   , body.position.z + sinf(i) * body.size };

        const Vector3 headVector = { body.position.x + cosf(i) * body.size
                                   , body.position.y + STAND_HEIGHT
                                   , body.position.z + sinf(i) * body.size };

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
    for (int i = 0; i < enemies.count; ++i)
    {
        Enemy enemy = enemies.enemies[i];
        Texture2D enemySprite;

        Vector2 enemyVector = Vector2Subtract(
            (Vector2){ player.body.position.x, player.body.position.z },
            (Vector2){ enemy.body.position.x, enemy.body.position.z }
        );

        Vector2 enemyDirVector = { enemy.data[1], enemy.data[2] };

        float playerEnemyAngle = Vector2Angle(enemyVector, enemyDirVector) * RAD2DEG;

        // if (debugMode) TraceLog(LOG_DEBUG, "DrawEnemies: playerEnemyAngle %02.02f", playerEnemyAngle);

        if      (fabsf(playerEnemyAngle) < 45.0f)
            enemySprite = enemySprites[enemy.type][enemy.status][RIGHT];

        else if (fabsf(playerEnemyAngle) > 135.0f)
            enemySprite = enemySprites[enemy.type][enemy.status][FRONT];

        else if (playerEnemyAngle < 0.0f)
            enemySprite = enemySprites[enemy.type][enemy.status][LEFT];

        else
            enemySprite = enemySprites[enemy.type][enemy.status][BACK];

        // if (debugMode && !IsTextureValid(enemySprite))
        //     TraceLog(LOG_ERROR, "DrawEnemies: trying to draw not existing texture %d %d %d", enemy.type, enemy.status, FRONT);

        DrawBillboard(camera, enemySprite, (Vector3){ enemy.body.position.x
                                                    , enemy.body.position.y + enemy.body.size * 4.0f
                                                    , enemy.body.position.z }
                                         , 2.0f, WHITE);
        if (debugMode)
        {
            DrawBodyMapCollision(enemies.enemies->body);
        }
    }
}


static void DrawItems(Camera camera)
{
    for (int i = 0; i < items.count; ++i)
    {
        if (items.items[i].status == PICKED_UP) continue;
        DrawBillboard(camera, itemSprites[items.items[i].type]
                     , (Vector3){ items.items[i].pos.x
                                , items.items[i].hitbox.max.y * 0.25f
                                , items.items[i].pos.z }
                     , 1.0f, WHITE);

        if (debugMode)
        {
            DrawBoundingBox(items.items[i].hitbox, GREEN);
        }
    }
}


static void UpdateItems(void)
{
    for (int i = 0; i < items.count; ++i)
    {
        if (items.items[i].status == PICKED_UP) continue;
        BoundingBox playerHitbox = { (Vector3){ player.body.position.x - player.body.size
                                              , player.body.position.y
                                              , player.body.position.z - player.body.size }
                                   , (Vector3){ player.body.position.x + player.body.size
                                              , player.body.position.y + STAND_HEIGHT
                                              , player.body.position.z + player.body.size } };

        if (CheckCollisionBoxes(items.items[i].hitbox, playerHitbox))
        {
            items.items[i].status = PICKED_UP;
            if (items.items[i].type < WeaponTypeSize)
                selectedWeapon = &items.items[i];
        }
    }
}


static void DrawWeapon(Texture2D currentAnimationFrame)
{
    DrawTextureEx(currentAnimationFrame, Vector2Zero(), 0, 3.0f, WHITE);
}


static Texture2D UpdateWeapon(bool leftMouseClicked)
{
    static double coldown = 0;
    static double frameTime = 0;
    static int frameCounter = 0;
    static WeaponAnimation currentAnimation = W_IDLE;

    if (selectedWeapon != NULL)
    {
        if (leftMouseClicked && coldown == 0)
        {
            coldown = GetTime();
            frameTime = GetTime();
            currentAnimation = W_ATTACK;
            frameCounter = 0;
        }
        else
        {
            // if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            //     ++frameCounter;

            static const double timeBetweenAnimationFrames = 1.0 / ANIMATION_FPS;
            if (GetTime() - frameTime > timeBetweenAnimationFrames)
            {
                ++frameCounter;
                frameTime = GetTime();
            }

            // Attack coldown
            if (coldown != 0)
            {
                switch (selectedWeapon->type) {
                case SLEDGE_HAMMER:
                    if (GetTime() - coldown > 1.5) coldown = 0;
                    break;
                case PISTOL:
                    break;
                default:
                    TraceLog(LOG_ERROR, "UpdateWeapon: this WeaponType does not exist!");
                }
            }
        }

        AnimationFrames currentAnimationFrames = weaponAnimations[selectedWeapon->type][currentAnimation];


        if (frameCounter >= currentAnimationFrames.count)
        {
            frameCounter = 0;
            // If attack animation ends, switch to idle
            if (currentAnimation == W_ATTACK)
            {
                currentAnimation = W_IDLE;
                currentAnimationFrames = weaponAnimations[selectedWeapon->type][currentAnimation];
            }
        }

        return currentAnimationFrames.frames[frameCounter];
    }

    static const Texture2D nullTexture = { 0, 0, 0, 0, 0 };
    return nullTexture;
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
        DrawModelWires(level, Vector3Zero(), 1.0f, BLACK);
    }

    rlEnableBackfaceCulling();

    DrawEnemies(camera);
    DrawItems(camera);
}
