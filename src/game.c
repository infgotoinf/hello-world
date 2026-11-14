#include "include/game.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "include/global_vars.h"
#include "include/pause.h"


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
#define CROUCH_HEIGHT    0.5f
#define STAND_HEIGHT     1.5f
#define FOV             80.0f

#define DEFAULT_BODY_SIZE 0.25f

#define NORMALIZE_INPUT  0



// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    float size;
    bool isGrounded;
} Body;



static Vector2 sensitivity = { 0.001f, 0.001f };

static Body player = { 0 };
static Vector2 lookRotation = { 0 };
static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;
static Vector2 lean = { 0 };
static Camera camera = { 0 };

static bool wasGravePressed = false;
static bool prevFrameCrouchHold = false;

static RenderTexture2D target;

static Texture2D self_idle;

static Model map;



static void DrawLevel(void);
static void UpdateCameraFPS(void);
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool *crouchHold);



bool LoadGame(void)
{
    self_idle = LoadTexture("assets/art/sprites/entities/self/idle.png");
    if (!IsTextureValid(self_idle)) {
        printf("Couldn't load Texture!\n");
        return false;
    }
    map = LoadModel("assets/maps/map.glb");
    if (!IsModelValid(map)) {
        printf("Couldn't load Model!\n");
        return false;
    }
    target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    camera.fovy = FOV;
    camera.projection = CAMERA_PERSPECTIVE;
    BoundingBox box = GetMeshBoundingBox(map.meshes[1]);
    player.position = (Vector3){
        (box.min.x + box.max.x) * 0.5f,
        (box.min.x + box.max.x) * 0.5f,
        (box.min.z + box.max.z) * 0.5f
    };
    player.size = DEFAULT_BODY_SIZE;
    camera.position = (Vector3){
        player.position.x,
        player.position.y + headLerp,
        player.position.z,
    };
    UpdateCameraFPS(); // Update camera parameters

    return true;
}


void UnloadGame(void)
{
    UnloadTexture(self_idle);
    UnloadModel(map);
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
            BeginTextureMode(target);
                ClearBackground(RAYWHITE);
                DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height },
                        (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

                DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){ 0, 0, 0, 100 });
            EndTextureMode();

            BeginDrawing();
                DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height },
                        (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);
            EndDrawing();
        }

        prevFramePaused = isPaused;
    }

    if (IsKeyPressed(KEY_ESCAPE))
        isPaused = !isPaused;

    if (IsKeyPressed(KEY_GRAVE))
        wasGravePressed = !wasGravePressed;

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
            break;
        case MAIN_MENU:
            break;
        case EXIT_TO_DESKTOP:
            break;
        }
    }
    else
    {
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
        UpdateBody(&player, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), &crouching);
        prevFrameCrouchHold = crouching;

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (crouching ? CROUCH_HEIGHT : STAND_HEIGHT), 20.0f*delta);
        camera.position = (Vector3){
            player.position.x + cameraPosAdd.x,
            player.position.y + headLerp,
            player.position.z,
        };

        if (player.isGrounded && ((forward != 0) || (sideway != 0)))
        {
            headTimer += delta*3.0f;
            walkLerp = Lerp(walkLerp, 1.0f, 10.0f*delta);
            camera.fovy = Lerp(camera.fovy, FOV - 5.0f, 5.0f*delta);
        }
        else
        {
            walkLerp = Lerp(walkLerp, 0.0f, 10.0f*delta);
            camera.fovy = Lerp(camera.fovy, FOV, 5.0f*delta);
        }

        lean.x = Lerp(lean.x, sideway*0.02f, 10.0f*delta);
        lean.y = Lerp(lean.y, forward*0.015f, 10.0f*delta);

        UpdateCameraFPS();
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                DrawLevel();
            EndMode3D();

            // Draw info box
            if (!wasGravePressed)
            {
                DrawRectangle(5, 5, 330, 75, Fade(SKYBLUE, 0.5f));
                DrawRectangleLines(5, 5, 330, 75, BLUE);

                DrawText("Camera controls:", 15, 15, 10, BLACK);
                DrawText("- Move keys: W, A, S, D, Space, Left-Ctrl", 15, 30, 10, BLACK);
                DrawText("- Look around: mouse", 15, 45, 10, BLACK);
                DrawText(TextFormat("- Velocity Len: (%06.3f)", Vector2Length((Vector2){ player.velocity.x, player.velocity.z })), 15, 60, 10, BLACK);
                DrawText(TextFormat("Mouse Pos: %03.02f %03.02f", GetMouseDelta().x, GetMouseDelta().y), 100, 100, 10, PURPLE);
                DrawText(TextFormat("Player Pos: %03.02f %03.02f", camera.position.x, camera.position.y), 100, 120, 10, PURPLE);
                DrawText(TextFormat("Player isGrounded: %d", player.isGrounded), 100, 140, 10, PURPLE);
                DrawText(TextFormat("%02.02f %02.02f %02.02f", player.dir.x, player.dir.y, player.dir.z), 100, 160, 10, PURPLE);
                DrawText("` TO TURN OFF DEBUG STUFF", 350, 50, 30, RED);

                static const int numberOfDirections = 8;
                // Using radians instead of degrees
                // 360 degrees = 2 * PI radians
                // This is because cos() and sin() expect angles in radians
                static const float rayAngleDifference = 2 * PI / (float)numberOfDirections;
                static const float radius = 75.0f;

                static const Vector2 center = { WINDOW_WIDTH - 100, 100 };

                for (float i = 0; i < 2 * PI; i += rayAngleDifference)
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
    static const int numberOfDirections = 8;
    // Using radians instead of degrees
    // 360 degrees = 2 * PI radians
    // This is because cos() and sin() expect angles in radians
    static const float rayAngleDifference = 2*PI / (float)numberOfDirections;
    // (Sorry for duplicating this for loop i'm cringe
    if (prevFrameCrouchHold == true && *crouchHold == false)
    {
        for (float i = 0; i < 2 * PI; i += rayAngleDifference)
        {
            const Ray heightRay = { (Vector3){ body->position.x + cos(i)*(body->size)
                                             , body->position.y + CROUCH_HEIGHT
                                             , body->position.z + sin(i)*(body->size) }
                                  , (Vector3){ 0
                                             , STAND_HEIGHT - CROUCH_HEIGHT
                                             , 0 } };
            const RayCollision crouchCeilingCollision =
                    GetRayCollisionMesh(heightRay, map.meshes[0], map.transform);


            if (crouchCeilingCollision.hit && crouchCeilingCollision.distance < STAND_HEIGHT - CROUCH_HEIGHT)
            {
                *crouchHold = true;
                break;
            }
        }
    }

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
    for (float i = 0; i < 2 * PI; i += rayAngleDifference)
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

        const RayCollision downWallCollision = GetRayCollisionMesh(kneeRay, map.meshes[0], map.transform);
        if (downWallCollision.hit && downWallCollision.distance < body->size)
            collidingRay = &downWallCollision;

        const RayCollision upWallCollision   = GetRayCollisionMesh(headRay, map.meshes[0], map.transform);
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
                                    , (Vector3){ 0, -1000, 0 } };

            const Ray upwardRay   = { (Vector3){ body->position.x + cos(i)*(body->size - 0.001f)
                                               , body->position.y
                                               , body->position.z + sin(i)*(body->size - 0.001f) }
                                    , (Vector3){ 0, 1000, 0 } };

            const RayCollision floorCollision   = GetRayCollisionMesh(downwardRay, map.meshes[0]
                                                                                 , map.transform);
            const RayCollision ceilingCollision = GetRayCollisionMesh(upwardRay  , map.meshes[0]
                                                                                 , map.transform);

            if (ceilingCollision.hit && ceilingCollision.distance <= 0.001)
            {
                body->position.y = ceilingCollision.point.y;
                body->velocity.y = 0.0f;
                body->isGrounded = true;
                skipGroundCollisionCheck = true;
            }
            else if (floorCollision.hit && floorCollision.distance * 1000 >= 0.01) body->isGrounded = false;
            else {
                body->isGrounded = true;
                skipGroundCollisionCheck = true;
            }
        }
    }
}

// Update camera for FPS behaviour
static void UpdateCameraFPS(void)
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
    camera.up = Vector3RotateByAxisAngle(up, pitch, headSin*stepRotation + lean.x);

    // Camera BOB
    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    Vector3 bobbing = Vector3Scale(right, headSin*bobSide);
    bobbing.y = fabsf(headCos*bobUp);

    camera.position = Vector3Add(camera.position, Vector3Scale(bobbing, walkLerp));
    camera.target = Vector3Add(camera.position, pitch);
}

// Draw game level
static void DrawLevel(void)
{
    rlDisableBackfaceCulling();
    DrawMesh(map.meshes[0], map.materials[0], map.transform);
    DrawModelWires(map, Vector3Zero(), 1.0f, BLACK);
    rlEnableBackfaceCulling();

    BoundingBox box = GetMeshBoundingBox(map.meshes[2]);

    DrawBillboard(camera, self_idle, (Vector3){
                                         (box.min.x + box.max.x) * 0.5f,
                                         (box.min.y + box.max.y) * 0.5f,
                                         (box.min.z + box.max.z) * 0.5f
                                     }, 3, WHITE);

    if (!wasGravePressed)
    {
        static const int numberOfDirections = 8;
        // Using radians instead of degrees
        // 360 degrees = 2 * PI radians
        // This is because cos() and sin() expect angles in radians
        static const float rayAngleDifference = 2 * PI / (float)numberOfDirections;

        for (float i = 0; i < 2 * PI; i += rayAngleDifference)
        {
            // https://gamemath.com/book/cartesianspace.html#trig_functins
            // cos(angle) = x, sin(angle) = y
            // Ray ray = { (Vector3){ player.position.x
            //                      , player.position.y + CROUCH_HEIGHT
            //                      , player.position.z }
            //           , (Vector3){ cos(i) * player.size
            //                      , 0
            //                      , sin(i) * player.size } };
            // DrawRay(ray, BLUE);

            const Vector3 kneeVector = { player.position.x + cos(i) * player.size
                                       , player.position.y + CROUCH_HEIGHT
                                       , player.position.z + sin(i) * player.size };

            const Vector3 headVector = { player.position.x + cos(i) * player.size
                                       , player.position.y + STAND_HEIGHT
                                       , player.position.z + sin(i) * player.size };

            DrawSphere(kneeVector, 0.01f, BLACK);
            DrawSphere(headVector, 0.01f, BLACK);
        }

        DrawRay((Ray){ (Vector3){ player.position.x
                                , player.position.y + CROUCH_HEIGHT
                                , player.position.z }
                     , player.dir }, ORANGE);

        DrawRay((Ray){ (Vector3){ player.position.x
                                , player.position.y + CROUCH_HEIGHT
                                , player.position.z }
                     , player.velocity }, YELLOW);
    }
}
