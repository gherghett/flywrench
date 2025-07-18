#include "screen_gameplay.h"
#include "screen_manager.h"
#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WING_WIDTH 40
#define WING_THICKNESS 4.0f
#define SEGMENT_THICKNESS 10.0f
#define MAX_SEGMENTS 100
#define ROT_SPEED 300.0
#define GRAPH_WIDTH 300
#define GRAPH_HEIGHT 120
#define GRAPH_SAMPLES 150
#define GRAPH_DISPLAY_SAMPLES 50

typedef struct {
    Vector2 start;
    Vector2 end;
} LineSegment;

typedef struct {
    LineSegment segments[MAX_SEGMENTS];
    int segmentCount;
    Vector2 goal;
} Level;

enum EditMode {
    EDIT_LINES_ADD,
    EDIT_LINES_REMOVE,
    EDIT_GOAL_PLACE
};

// Static variables for gameplay state
static Vector2 playerPos;
static float playerRot;
static float flapAmount;
static float flapVelocity;
static Vector2 playerVelocity;
static Camera2D camera;
static Vector2 leftWing;
static Vector2 rightWing;
static Vector2 clickCircle;
static Level currentLevelData;
static int currentLevel;
static bool editMode;
static Vector2 editPos;
static bool editModeStartOfCurrentSegment;
static Vector2 editModeStartPosition;
static enum EditMode editModeCurrent;
static bool graphEnabled;
static bool debugInfoEnabled;
static float flapVelocityHistory[GRAPH_SAMPLES];
static float flapAmountHistory[GRAPH_SAMPLES];
static float playerVelocityMagnitudeHistory[GRAPH_SAMPLES];
static int graphIndex;
static float graphUpdateTimer;

const char* EditModeToString(enum EditMode mode) {
    switch(mode) {
        case EDIT_LINES_ADD: return "ADD LINES";
        case EDIT_LINES_REMOVE: return "REMOVE LINES";
        case EDIT_GOAL_PLACE: return "PLACE GOAL";
        default: return "UNKNOWN";
    }
}

void save_level(Level* level, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) return;
    fwrite(level, sizeof(Level), 1, file);
    fclose(file);
}

int load_level(Level* level, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        memset(level, 0, sizeof(Level));
        return 0;
    }
    fread(level, sizeof(Level), 1, file);
    fclose(file);
    return 0;
}

int orientation(Vector2 a, Vector2 b, Vector2 c) {
    float val = (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
    if (val == 0.0) return 0;
    if (val > 0) return 1;
    return 2;
}

bool intersects(Vector2 p1, Vector2 p2, Vector2 q1, Vector2 q2) {
    int o1 = orientation(p1, p2, q1);
    int o2 = orientation(p1, p2, q2);
    int o3 = orientation(q1, q2, p1);
    int o4 = orientation(q1, q2, p2);
    return (o1 != o2) && (o3 != o4);
}

bool CollisionWithLine(Vector2 playerPos, Vector2 leftWing, Vector2 rightWing, Vector2 p1, Vector2 p2) {
    if (intersects(playerPos, leftWing, p1, p2)) return true;
    if (intersects(playerPos, rightWing, p1, p2)) return true;
    Vector2 offPlayerPos = {playerPos.x + 10, playerPos.y + 10};
    if (intersects(offPlayerPos, leftWing, p1, p2)) return true;
    if (intersects(offPlayerPos, rightWing, p1, p2)) return true;
    return false;
}

void DrawGraph(float* data, int samples, int startIndex, float x, float y, float width, float height, Color color, const char* label) {
    int displaySamples = GRAPH_DISPLAY_SAMPLES;
    int displayStartIndex = (startIndex - displaySamples + samples) % samples;
    
    float minVal = data[displayStartIndex];
    float maxVal = data[displayStartIndex];
    for (int i = 0; i < displaySamples; i++) {
        int idx = (displayStartIndex + i) % samples;
        if (data[idx] < minVal) minVal = data[idx];
        if (data[idx] > maxVal) maxVal = data[idx];
    }
    
    float range = maxVal - minVal;
    if (range < 0.1f) range = 0.1f;
    minVal -= range * 0.1f;
    maxVal += range * 0.1f;
    range = maxVal - minVal;
    
    DrawRectangle(x, y, width, height, (Color){40, 40, 40, 200});
    DrawRectangleLines(x, y, width, height, WHITE);
    
    DrawText(label, x + 5, y + 5, 12, WHITE);
    DrawText(TextFormat("%.2f", maxVal), x + 5, y + 20, 10, LIGHTGRAY);
    DrawText(TextFormat("%.2f", minVal), x + 5, y + height - 15, 10, LIGHTGRAY);
    
    for (int i = 1; i < displaySamples; i++) {
        int prevIndex = (displayStartIndex + i - 1) % samples;
        int currIndex = (displayStartIndex + i) % samples;
        
        float x1 = x + ((float)(i - 1) / (displaySamples - 1)) * width;
        float y1 = y + height - ((data[prevIndex] - minVal) / range) * height;
        float x2 = x + ((float)i / (displaySamples - 1)) * width;
        float y2 = y + height - ((data[currIndex] - minVal) / range) * height;
        
        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, color);
    }
}

void ScreenGameplay_Init(void) {
    playerPos = (Vector2){100, 100};
    playerRot = 0.0;
    flapAmount = 0.0;
    flapVelocity = 0.0;
    playerVelocity = (Vector2){0, 0};
    
    camera.target = (Vector2){playerPos.x + 20.0f, playerPos.y + 20.0f};
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 0.5f;
    
    leftWing = (Vector2){-WING_WIDTH, 0};
    rightWing = (Vector2){WING_WIDTH, 0};
    clickCircle = (Vector2){0};
    
    currentLevel = 0;
    editMode = false;
    editModeStartOfCurrentSegment = true;
    editModeStartPosition = (Vector2){0};
    editModeCurrent = EDIT_LINES_ADD;
    
    graphEnabled = false;
    debugInfoEnabled = 0;

    memset(flapVelocityHistory, 0, sizeof(flapVelocityHistory));
    memset(flapAmountHistory, 0, sizeof(flapAmountHistory));
    memset(playerVelocityMagnitudeHistory, 0, sizeof(playerVelocityMagnitudeHistory));
    graphIndex = 0;
    graphUpdateTimer = 0.0f;
    
    load_level(&currentLevelData, TextFormat("level%d", currentLevel));
}

void ScreenGameplay_Update(void) {
    float delta = GetFrameTime();
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    
    // Handle back to menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        ChangeToScreen(SCREEN_MENU);
        return;
    }
    
    // Update graph data
    graphUpdateTimer += delta;
    if (graphUpdateTimer >= 0.05f) {
        graphUpdateTimer = 0.0f;
        flapVelocityHistory[graphIndex] = flapVelocity;
        flapAmountHistory[graphIndex] = flapAmount;
        playerVelocityMagnitudeHistory[graphIndex] = Vector2Length(playerVelocity);
        graphIndex = (graphIndex + 1) % GRAPH_SAMPLES;
    }
    
    if (IsKeyPressed(KEY_P)) {
        editMode = !editMode;
        if (editMode) {
            editPos = playerPos;
        }
    }
    
    if (IsKeyPressed(KEY_G)) {
        graphEnabled = !graphEnabled;
    }
    if (IsKeyPressed(KEY_H)) {
        debugInfoEnabled = !debugInfoEnabled;
    }
    
    if (IsKeyPressed(KEY_SPACE)) {
        playerRot = 0.0;
        playerPos = (Vector2){100, 100};
        playerVelocity = (Vector2){0, 0};
    }
    
    if (!editMode) {
        // PLAYMODE
        if (IsKeyDown(KEY_RIGHT)) {
            playerRot += ROT_SPEED * delta;
        }
        
        if (IsKeyDown(KEY_LEFT)) {
            playerRot -= ROT_SPEED * delta;
        }
        
        if (IsKeyDown(KEY_DOWN)) {
            if (flapAmount < 90.0) {
                flapVelocity += 30.0 * delta + flapVelocity;
            }
            if (flapVelocity > 2000.0) flapVelocity = 2000.0;
            flapAmount += flapVelocity;
            if (flapAmount >= 90.0) {
                flapAmount = 90.0;
                flapVelocity = 0;
            }
        } else {
            flapVelocity = 0;
            flapAmount -= 500.0 * delta;
            if (flapAmount < 0.0) flapAmount = 0.0;
        }
        
        Vector2 velocityFromFlapVector = Vector2Rotate((Vector2){0, -flapVelocity * 8 * delta}, DEG2RAD * playerRot);
        playerVelocity = Vector2Add(velocityFromFlapVector, playerVelocity);
        
        // gravity
        playerVelocity = Vector2Add(playerVelocity, (Vector2){0, 10 * delta});
        
        playerPos = Vector2Add(playerPos, playerVelocity);
        
        // Camera follows player with smooth movement
        float distance = Vector2Distance(playerPos, camera.target);
        if (distance > 100.0) {
            Vector2 difference = Vector2Subtract(playerPos, camera.target);
            Vector2 direction = Vector2Normalize(difference);
            Vector2 targetPos = Vector2Add(camera.target, Vector2Scale(direction, distance - 100.0));
            camera.target = targetPos;
        }
        
        leftWing = (Vector2){-WING_WIDTH, 0};
        rightWing = (Vector2){WING_WIDTH, 0};
        leftWing = Vector2Rotate(leftWing, playerRot * DEG2RAD);
        rightWing = Vector2Rotate(rightWing, playerRot * DEG2RAD);
        leftWing = Vector2Rotate(leftWing, -flapAmount * DEG2RAD);
        rightWing = Vector2Rotate(rightWing, flapAmount * DEG2RAD);
        leftWing = Vector2Add(leftWing, playerPos);
        rightWing = Vector2Add(rightWing, playerPos);
        
        // Collision detection
        for (int i = 0; i < currentLevelData.segmentCount; i++) {
            if (CollisionWithLine(playerPos, leftWing, rightWing,
                                currentLevelData.segments[i].start, currentLevelData.segments[i].end)) {
                playerRot = 0.0;
                playerPos = (Vector2){100, 50};
                playerVelocity = (Vector2){0, 0};
                leftWing = (Vector2){-WING_WIDTH, 0};
                rightWing = (Vector2){WING_WIDTH, 0};
                leftWing = Vector2Rotate(leftWing, playerRot * DEG2RAD);
                rightWing = Vector2Rotate(rightWing, playerRot * DEG2RAD);
                leftWing = Vector2Rotate(leftWing, -flapAmount * DEG2RAD);
                rightWing = Vector2Rotate(rightWing, flapAmount * DEG2RAD);
                leftWing = Vector2Add(leftWing, playerPos);
                rightWing = Vector2Add(rightWing, playerPos);
            }
        }
        
        // Check goal collision
        if (currentLevelData.goal.x != 0 || currentLevelData.goal.y != 0) {
            if (Vector2Distance(playerPos, currentLevelData.goal) < 40) {
                // Goal reached! Load next level
                currentLevel++;
                load_level(&currentLevelData, TextFormat("level%d", currentLevel));
                playerRot = 0.0;
                playerPos = (Vector2){100, 100};
                playerVelocity = (Vector2){0, 0};
            }
        }
    } else {
        // EDITMODE
        int editSpeed = 400;
        if (IsKeyDown(KEY_LEFT)) editPos.x -= editSpeed * delta;
        if (IsKeyDown(KEY_RIGHT)) editPos.x += editSpeed * delta;
        if (IsKeyDown(KEY_UP)) editPos.y -= editSpeed * delta;
        if (IsKeyDown(KEY_DOWN)) editPos.y += editSpeed * delta;
        
        if (IsKeyPressed(KEY_S)) {
            save_level(&currentLevelData, TextFormat("level%d", currentLevel));
        }
        if (IsKeyPressed(KEY_D)) {
            editModeCurrent = (editModeCurrent + 1) % 3;
        }
        
        if (IsKeyPressed(KEY_N)) {
            currentLevel++;
            load_level(&currentLevelData, TextFormat("level%d", currentLevel));
        }
        if (IsKeyPressed(KEY_B)) {
            currentLevel--;
            load_level(&currentLevelData, TextFormat("level%d", currentLevel));
        }
        
        switch (editModeCurrent) {
            case EDIT_LINES_ADD:
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 pressedPos = GetScreenToWorld2D(GetMousePosition(), camera);
                    clickCircle = pressedPos;
                    if (editModeStartOfCurrentSegment) {
                        editModeStartPosition = pressedPos;
                    } else {
                        currentLevelData.segments[currentLevelData.segmentCount++] = (LineSegment){editModeStartPosition, pressedPos};
                    }
                    editModeStartOfCurrentSegment = !editModeStartOfCurrentSegment;
                }
                break;
            case EDIT_LINES_REMOVE:
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int segmentClicked = -1;
                    for (int i = 0; i < currentLevelData.segmentCount; i++) {
                        if (CollisionWithLine(mousePos,
                                            (Vector2){mousePos.x + 5, mousePos.y + 5},
                                            (Vector2){mousePos.x - 5, mousePos.y - 5},
                                            currentLevelData.segments[i].start, currentLevelData.segments[i].end)) {
                            segmentClicked = i;
                            break;
                        }
                    }
                    if (segmentClicked != -1) {
                        if (currentLevelData.segmentCount == 1) break;
                        currentLevelData.segments[segmentClicked] = currentLevelData.segments[currentLevelData.segmentCount - 1];
                        currentLevelData.segmentCount--;
                        save_level(&currentLevelData, TextFormat("level%d", currentLevel));
                    }
                }
                break;
            case EDIT_GOAL_PLACE:
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    currentLevelData.goal = mousePos;
                }
                break;
            default:
                break;
        }
        
        camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        
        camera.target = editPos;
    }
}

void ScreenGameplay_Draw(void) {
    BeginDrawing();
    
    ClearBackground((Color){20,20,20,255});
    
    BeginMode2D(camera);
    
    // Draw player
    DrawLineEx(leftWing, playerPos, WING_THICKNESS, WHITE);
    DrawLineEx(playerPos, rightWing, WING_THICKNESS, WHITE);
    
    DrawCircleV(clickCircle, 20, RED);
    
    // Draw level segments
    for (int i = 0; i < currentLevelData.segmentCount; i++) {
        DrawLineEx(currentLevelData.segments[i].start, currentLevelData.segments[i].end, SEGMENT_THICKNESS, RAYWHITE);
    }
    
    // Draw goal
    if (currentLevelData.goal.x != 0 || currentLevelData.goal.y != 0) {
        DrawCircleV(currentLevelData.goal, 30, GREEN);
        DrawCircleV(currentLevelData.goal, 25, DARKGREEN);
    }
    
    if (editMode) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
        
        switch (editModeCurrent) {
            case EDIT_LINES_ADD:
                break;
            case EDIT_LINES_REMOVE:
                for (int i = 0; i < currentLevelData.segmentCount; i++) {
                    if (CollisionWithLine(mousePos, (Vector2){mousePos.x + 5, mousePos.y + 5}, (Vector2){mousePos.x - 5, mousePos.y - 5},
                                        currentLevelData.segments[i].start, currentLevelData.segments[i].end)) {
                        DrawLineEx(currentLevelData.segments[i].start, currentLevelData.segments[i].end, SEGMENT_THICKNESS, YELLOW);
                        DrawCircleV(currentLevelData.segments[i].start, 20, YELLOW);
                        DrawCircleV(currentLevelData.segments[i].end, 20, YELLOW);
                    }
                }
                break;
            case EDIT_GOAL_PLACE:
                // Show goal preview at mouse position
                DrawCircleV(mousePos, 30, (Color){0, 255, 0, 100});
                DrawCircleV(mousePos, 25, (Color){0, 150, 0, 100});
                break;
            default:
                break;
        }
    }
    
    EndMode2D();
    int textY = 10;
    int textLineHeight = 25;
    if(debugInfoEnabled) {
        // Draw HUD text with debug info
        DrawText(TextFormat("Flap Velocity: %.2f", flapVelocity), 10, textY+=textLineHeight, 20, WHITE);
        DrawText(TextFormat("Flap Amount: %.2f", flapAmount), 10, textY+=textLineHeight, 20, WHITE);
        DrawText(TextFormat("Player Velocity: (%.2f, %.2f)", playerVelocity.x, playerVelocity.y), 10, textY+=textLineHeight, 20, WHITE);
        DrawText(TextFormat("Player Rotation: %.2f", playerRot), 10, textY+=textLineHeight, 20, WHITE);
        DrawText(TextFormat("Current Level: %d", currentLevel), 10, textY+=textLineHeight, 20, WHITE);
        DrawText(TextFormat("Edit Mode: %s", EditModeToString(editModeCurrent)), 10, textY+=textLineHeight, 20, WHITE);
    }
    // DrawText("Press ESC to return to menu", 10, textY+=textLineHeight, 16, LIGHTGRAY);
    
    if (graphEnabled) {
        // Draw graphs
        float graphX = GetScreenWidth() - GRAPH_WIDTH - 10;
        float graphY = 10;
        
        DrawGraph(flapVelocityHistory, GRAPH_SAMPLES, graphIndex, graphX, graphY, GRAPH_WIDTH, GRAPH_HEIGHT, RED, "Flap Velocity");
        DrawGraph(flapAmountHistory, GRAPH_SAMPLES, graphIndex, graphX, graphY + GRAPH_HEIGHT + 5, GRAPH_WIDTH, GRAPH_HEIGHT, BLUE, "Flap Amount");
        DrawGraph(playerVelocityMagnitudeHistory, GRAPH_SAMPLES, graphIndex, graphX, graphY + 2 * (GRAPH_HEIGHT + 5), GRAPH_WIDTH, GRAPH_HEIGHT, GREEN, "Player Speed");
        
        if (editMode) {
            DrawText("EDIT MODE - Press P to toggle", 10, GetScreenHeight() - 30, 20, YELLOW);
            DrawText("S to save segments", 10, GetScreenHeight() - 55, 20, YELLOW);
        } else {
            DrawText("PLAY MODE - Press P to toggle", 10, GetScreenHeight() - 30, 20, GREEN);
        }
    }
    
    EndDrawing();
}

void ScreenGameplay_Unload(void) {
    // Clean up gameplay resources
    // No dynamic memory to free in this implementation
}
