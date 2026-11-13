#include "include/game.h"

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

// "directional billboard" example: https://www.raylib.com/examples/models/loader.html?name=models_directional_billboard
// Zlib license


// Movement constants
#define GRAVITY         48.0f
#define MAX_SPEED        7.0f
#define CROUCH_SPEED     5.0f
#define JUMP_FORCE      12.0f
#define MAX_ACCEL       50.0f
// Grounded drag
#define FRICTION        0.86f
// Increasing air drag, increases strafing speed
#define AIR_DRAG        0.98f
// Responsiveness for turning movement direction to looked direction
#define CONTROL         15.0f
#define CROUCH_HEIGHT    1.0f
#define STAND_HEIGHT     2.0f
#define FOV             80.0f

#define NORMALIZE_INPUT  0



// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
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

static Texture2D self_idle;

static Model map;


static void DrawLevel(void);
static void UpdateCameraFPS(Camera *camera);
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);



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
    camera.position = (Vector3){
        player.position.x,
        player.position.y + headLerp,
        player.position.z,
    };
    UpdateCameraFPS(&camera); // Update camera parameters

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

    if (isPaused)
    {
        isPaused = DrawPause();
    }
    else
    {
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mouseDelta = GetMouseDelta();
        lookRotation.x -= mouseDelta.x*sensitivity.x;
        lookRotation.y += mouseDelta.y*sensitivity.y;

        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        bool crouching = IsKeyDown(KEY_LEFT_CONTROL);
        UpdateBody(&player, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), crouching);

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (crouching ? CROUCH_HEIGHT : STAND_HEIGHT), 20.0f*delta);
        camera.position = (Vector3){
            player.position.x,
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

        UpdateCameraFPS(&camera);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                DrawLevel();
            EndMode3D();

            // Draw info box
            DrawRectangle(5, 5, 330, 75, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(5, 5, 330, 75, BLUE);

            DrawText("Camera controls:", 15, 15, 10, BLACK);
            DrawText("- Move keys: W, A, S, D, Space, Left-Ctrl", 15, 30, 10, BLACK);
            DrawText("- Look around: arrow keys or mouse", 15, 45, 10, BLACK);
            DrawText(TextFormat("- Velocity Len: (%06.3f)", Vector2Length((Vector2){ player.velocity.x, player.velocity.z })), 15, 60, 10, BLACK);
            DrawText(TextFormat("Mouse Pos: %03.02f %03.02f", GetMouseDelta().x, GetMouseDelta().y), 100, 100, 10, PURPLE);
            DrawText(TextFormat("Player Pos: %03.02f %03.02f", camera.position.x, camera.position.y), 100, 120, 10, PURPLE);
            DrawText(TextFormat("Player isGrounded: %d", player.isGrounded), 100, 140, 10, PURPLE);
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
void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward };

#if defined(NORMALIZE_INPUT)
    // Slow down diagonal movement
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);
#endif

    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY*delta;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;

        // Sound can be played at this moment
        //SetSoundPitch(fxJump, 1.0f + (GetRandomValue(-100, 100)*0.001));
        //PlaySound(fxJump);
    }

    Vector3 front = (Vector3){ sinf(rot), 0.f, cosf(rot) };
    Vector3 right = (Vector3){ cosf(-rot), 0.f, sinf(-rot) };

    Vector3 desiredDir = (Vector3){ input.x*right.x + input.y*front.x, 0.0f, input.x*right.z + input.y*front.z, };
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
    float maxSpeed = (crouchHold? CROUCH_SPEED : MAX_SPEED);
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL*delta);
    hvel.x += body->dir.x*accel;
    hvel.z += body->dir.z*accel;

    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    body->position.x += body->velocity.x*delta;
    body->position.y += body->velocity.y*delta;
    body->position.z += body->velocity.z*delta;

    //       ^
    //   ()  |  - floorCollision
    //  /|\  |
    //  /\   +
    //       |  - floorDistance
    //       |
    //       V

    RayCollision floorDistance = GetRayCollisionMesh((Ray){ player.position, (Vector3){ player.position.x, -1000, player.position.z} }, map.meshes[0], map.transform);
    if (floorDistance.hit && floorDistance.distance >= 0.01)
        body->isGrounded = true;
    else
        body->isGrounded = false;

    RayCollision floorCollision = GetRayCollisionMesh((Ray){ player.position, (Vector3){ player.position.x, +1000, player.position.z} }, map.meshes[0], map.transform);
    if (floorCollision.hit && floorCollision.distance <= 0.001)
    {
        body->position.y = floorCollision.point.y;
        body->velocity.y = 0.0f;
        body->isGrounded = true;
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

// Draw game level
static void DrawLevel(void)
{
    rlDisableBackfaceCulling();
    DrawModel(map, (Vector3){ 0.0f }, 1.0f, WHITE);
    DrawModelWires(map, (Vector3){ 0.0f }, 1.0f, BLACK);
    rlEnableBackfaceCulling();

    BoundingBox box = GetMeshBoundingBox(map.meshes[2]);
    DrawBillboard(camera, self_idle, (Vector3){
                                         (box.min.x + box.max.x) * 0.5f,
                                         (box.min.y + box.max.y) * 0.5f,
                                         (box.min.z + box.max.z) * 0.5f
                                     }, 3, WHITE);
}
