#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"

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

enum EditMode {
    EDIT_LINES_ADD,
    EDIT_LINES_REMOVE,
    EDIT_GOAL_PLACE
};

const char* EditModeToString(enum EditMode mode) {
    switch(mode) {
        case EDIT_LINES_ADD: return "ADD LINES";
        case EDIT_LINES_REMOVE: return "REMOVE LINES";
        case EDIT_GOAL_PLACE: return "PLACE GOAL";
        default: return "UNKNOWN";
    }
}

void save_segments(LineSegment* segments, int count, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) return;

    fwrite(&count, sizeof(int), 1, file);  // write count
    fwrite(segments, sizeof(LineSegment), count, file);
    fclose(file);
}

int load_segments(LineSegment *segments, const char* filename, int* out_count) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        memset(segments, 0, MAX_SEGMENTS * sizeof(LineSegment));
        return 0;
    }

    fread(out_count, sizeof(int), 1, file);
    fread(segments, sizeof(LineSegment), *out_count, file);
    fclose(file);
    return 0;
}

int orientation(Vector2 a,  Vector2 b, Vector2 c){
    // Returns:
    // 0 → colinear
    // 1 → clockwise
    // 2 → counterclockwise
    float val = (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
    if (val == 0.0) return 0;
    if( val > 0 ) return 1;
    return 2;
}

bool intersects(Vector2 p1, Vector2 p2, Vector2 q1, Vector2 q2) {
    int o1 = orientation(p1, p2, q1);
    int o2 = orientation(p1, p2, q2);
    int o3 = orientation(q1, q2, p1);
    int o4 = orientation(q1, q2, p2);

    return (o1 != o2) && (o3 != o4);
}

bool CollisionWithLine(Vector2 playerPos, Vector2 leftWing, Vector2 rightWing, Vector2 p1, Vector2 p2)  {
    if(intersects(playerPos, leftWing, p1, p2)) return true;
    if(intersects(playerPos, rightWing, p1, p2)) return true;
    Vector2 offPlayerPos = {playerPos.x + 10, playerPos.y + 10};
    if(intersects(offPlayerPos, leftWing, p1, p2)) return true;
    if(intersects(offPlayerPos, rightWing, p1, p2)) return true; 
    return false;
}

// Vector2 *GetRotatedRectCorners(Rectangle rect, float angle) {
//     static Vector2 corners[4];
    
//     // Calculate the center of the rectangle
//     Vector2 center = {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
    
//     // Calculate the half dimensions
//     float halfWidth = rect.width / 2.0f;
//     float halfHeight = rect.height / 2.0f;
    
//     // Define the four corners relative to the center
//     Vector2 relativeCorners[4] = {
//         {-halfWidth, -halfHeight}, // Top-left
//         {halfWidth, -halfHeight},  // Top-right
//         {halfWidth, halfHeight},   // Bottom-right
//         {-halfWidth, halfHeight}   // Bottom-left
//     };
    
//     // Rotate each corner around the center and translate to world position
//     for (int i = 0; i < 4; i++) {
//         corners[i] = Vector2Rotate(relativeCorners[i], angle * DEG2RAD);
//         corners[i] = Vector2Add(corners[i], center);
//     }
    
//     return corners;
// }

void DrawGraph(float* data, int samples, int startIndex, float x, float y, float width, float height, Color color, const char* label) {
    // Use fewer samples for zoomed-in view
    int displaySamples = GRAPH_DISPLAY_SAMPLES;
    int displayStartIndex = (startIndex - displaySamples + samples) % samples;
    
    // Find min and max values for autoscaling (only in the display range)
    float minVal = data[displayStartIndex];
    float maxVal = data[displayStartIndex];
    for(int i = 0; i < displaySamples; i++) {
        int idx = (displayStartIndex + i) % samples;
        if(data[idx] < minVal) minVal = data[idx];
        if(data[idx] > maxVal) maxVal = data[idx];
    }
    
    // Add some padding to the scale
    float range = maxVal - minVal;
    if(range < 0.1f) range = 0.1f; // Prevent division by zero
    minVal -= range * 0.1f;
    maxVal += range * 0.1f;
    range = maxVal - minVal;
    
    // Draw background
    DrawRectangle(x, y, width, height, (Color){40, 40, 40, 200});
    DrawRectangleLines(x, y, width, height, WHITE);
    
    // Draw label
    DrawText(label, x + 5, y + 5, 12, WHITE);
    
    // Draw scale info
    DrawText(TextFormat("%.2f", maxVal), x + 5, y + 20, 10, LIGHTGRAY);
    DrawText(TextFormat("%.2f", minVal), x + 5, y + height - 15, 10, LIGHTGRAY);
    
    // Draw data points (only recent samples)
    for(int i = 1; i < displaySamples; i++) {
        int prevIndex = (displayStartIndex + i - 1) % samples;
        int currIndex = (displayStartIndex + i) % samples;
        
        float x1 = x + ((float)(i - 1) / (displaySamples - 1)) * width;
        float y1 = y + height - ((data[prevIndex] - minVal) / range) * height;
        float x2 = x + ((float)i / (displaySamples - 1)) * width;
        float y2 = y + height - ((data[currIndex] - minVal) / range) * height;
        
        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, color);
    }
}


int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    Vector2 playerPos = {100, 100};
    float playerRot = 0.0;
    float flapAmount = 0.0;
    float flapVelocity = 0.0;
    Vector2 playerVelocity = {0,0};



    InitWindow(screenWidth, screenHeight, "Flywrench");

    Camera2D camera = { 0 };
    camera.target = (Vector2){ playerPos.x + 20.0f, playerPos.y + 20.0f };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 0.7f;
    Vector2 leftWing = {-WING_WIDTH, 0};
    Vector2 rightWing = {WING_WIDTH, 0};

    Vector2 clickCircle = {0};

    LineSegment segments[MAX_SEGMENTS] = {0};
    int segmentCount = 0;

    //editmode
    bool editMode = false;
    Vector2 editPos;
    bool editModeStartOfCurrentSegment = true;
    Vector2 editModeStartPosition = {0};
    enum EditMode editModeCurrent = EDIT_LINES_ADD;

    
    // Graph data for visualization
    float flapVelocityHistory[GRAPH_SAMPLES] = {0};
    float flapAmountHistory[GRAPH_SAMPLES] = {0};
    float playerVelocityMagnitudeHistory[GRAPH_SAMPLES] = {0};
    int graphIndex = 0;
    float graphUpdateTimer = 0.0f;
    
    int currentLevel = 0;
    load_segments(segments, TextFormat("segments%d", currentLevel), &segmentCount);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        float delta =  GetFrameTime();

        // Update graph data
        graphUpdateTimer += delta;
        if(graphUpdateTimer >= 0.05f) { // Update every 50ms
            graphUpdateTimer = 0.0f;
            flapVelocityHistory[graphIndex] = flapVelocity;
            flapAmountHistory[graphIndex] = flapAmount;
            playerVelocityMagnitudeHistory[graphIndex] = Vector2Length(playerVelocity);
            graphIndex = (graphIndex + 1) % GRAPH_SAMPLES;
        }

        if(IsKeyPressed(KEY_P)) {
            editMode = !editMode;
            if(editMode) {
                editPos = playerPos;
            }
        }

        if(!editMode){
            // ----------------------PLAYMODE-----------------------
            if(IsKeyPressed(KEY_SPACE)) {
                playerRot = 0.0;
                playerPos = (Vector2){100, 100};
                playerVelocity = (Vector2){0,0};
            }
            
            if(IsKeyDown(KEY_RIGHT)) {
                playerRot += ROT_SPEED * delta;
            }

            if(IsKeyDown(KEY_LEFT)) {
                playerRot -= ROT_SPEED * delta;
            }

            if(IsKeyDown(KEY_DOWN)) {
                if(flapAmount < 90.0) {
                    flapVelocity += 30.0 * delta + flapVelocity ;
                }
                if(flapVelocity > 2000.0) flapVelocity = 2000.0;
                flapAmount += flapVelocity;
                if(flapAmount >= 90.0) {
                    flapAmount = 90.0;
                    flapVelocity = 0;
                }
            } else {
                flapVelocity = 0;
                flapAmount -= 500.0 * delta;
                if(flapAmount< 0.0) flapAmount = 0.0;
            }

            Vector2 velocityFromFlapVector = Vector2Rotate((Vector2){0, -flapVelocity*8*delta}, DEG2RAD*playerRot);

            playerVelocity = Vector2Add(velocityFromFlapVector, playerVelocity);

            //gravity
            playerVelocity = Vector2Add(playerVelocity, (Vector2){0,10*delta}); 


            playerPos = Vector2Add(playerPos, playerVelocity);
            
            // Camera follows player with smooth movement
            float distance = Vector2Distance(playerPos, camera.target);
            if(distance > 100.0) {
                Vector2 difference = Vector2Subtract(playerPos, camera.target);
                Vector2 direction = Vector2Normalize(difference);
                Vector2 targetPos = Vector2Add(camera.target, Vector2Scale(direction, distance - 100.0));
                camera.target = targetPos;
            }

            leftWing = (Vector2){-WING_WIDTH, 0};
            rightWing = (Vector2){WING_WIDTH, 0};
            leftWing = Vector2Rotate(leftWing, playerRot*DEG2RAD);
            rightWing = Vector2Rotate(rightWing, playerRot*DEG2RAD);
            leftWing = Vector2Rotate(leftWing, -flapAmount*DEG2RAD);
            rightWing = Vector2Rotate(rightWing, flapAmount*DEG2RAD);
            leftWing = Vector2Add(leftWing, playerPos);
            rightWing = Vector2Add(rightWing, playerPos);

            // Rectangle rect = {0,0,300,200};

            for(int i = 0; i < segmentCount; i++) {
                if(CollisionWithLine(playerPos, leftWing, rightWing, 
                    segments[i].start, segments[i].end)){
                    playerRot = 0.0;
                    playerPos = (Vector2){100, 50};
                    playerVelocity = (Vector2){0,0}; 
                    leftWing = (Vector2){-WING_WIDTH, 0};
                    rightWing = (Vector2){WING_WIDTH, 0};
                    leftWing = Vector2Rotate(leftWing, playerRot*DEG2RAD);
                    rightWing = Vector2Rotate(rightWing, playerRot*DEG2RAD);
                    leftWing = Vector2Rotate(leftWing, -flapAmount*DEG2RAD);
                    rightWing = Vector2Rotate(rightWing, flapAmount*DEG2RAD);
                    leftWing = Vector2Add(leftWing, playerPos);
                    rightWing = Vector2Add(rightWing, playerPos);
                }
            }

            //-------------------------------------------------
        } else {
            //-----------------editMode------------------------
            int editSpeed = 400;
            if(IsKeyDown(KEY_LEFT)) editPos.x -= editSpeed * delta;
            if(IsKeyDown(KEY_RIGHT)) editPos.x += editSpeed * delta;
            if(IsKeyDown(KEY_UP)) editPos.y -= editSpeed * delta;
            if(IsKeyDown(KEY_DOWN)) editPos.y += editSpeed * delta;

            if(IsKeyPressed(KEY_S)) {
                save_segments(segments, segmentCount, TextFormat("segments%d", currentLevel));
            }
            if(IsKeyPressed(KEY_D)){
                editModeCurrent = (editModeCurrent + 1) % 3;
            }

            if(IsKeyPressed(KEY_N)) {
                currentLevel++;
                load_segments(segments, TextFormat("segments%d", currentLevel), &segmentCount);
            }
            if(IsKeyPressed(KEY_B)) {
                currentLevel--;
                load_segments(segments, TextFormat("segments%d", currentLevel), &segmentCount);
            }
            switch(editModeCurrent) {
                case EDIT_LINES_ADD:
                    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        Vector2 pressedPos =  GetScreenToWorld2D(GetMousePosition(), camera);
                        clickCircle = pressedPos;
                        if(editModeStartOfCurrentSegment)
                        {
                            editModeStartPosition = pressedPos;
                        } else {
                            segments[segmentCount++] = (LineSegment){editModeStartPosition, pressedPos};
                        }
                        editModeStartOfCurrentSegment = !editModeStartOfCurrentSegment;
                    }
                break;
                case EDIT_LINES_REMOVE:
                    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        Vector2 mousePos =  GetScreenToWorld2D(GetMousePosition(), camera);
                        int segmentClicked = -1;
                        for(int i = 0; i < segmentCount; i++) {
                            if(CollisionWithLine(mousePos, 
                                (Vector2){mousePos.x + 5, mousePos.y + 5}, 
                                (Vector2){mousePos.x - 5, mousePos.y - 5}, 
                                segments[i].start, segments[i].end)){
                                 segmentClicked = i;
                                 break;
                            }
                        }
                        if(segmentClicked != -1) {
                            if(segmentCount == 1) break;
                            segments[segmentClicked] = segments[segmentCount-1];
                            segmentCount--;
                            save_segments(segments, segmentCount, TextFormat("segments%d", currentLevel));
                        }
                    }
                break;
                case EDIT_GOAL_PLACE:
                break;
                default: break;
            }

            camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove()*0.1f));
            if (camera.zoom > 3.0f) camera.zoom = 3.0f;
            else if (camera.zoom < 0.1f) camera.zoom = 0.1f;

            camera.target = editPos;
            //-------------------------------------------------
        }

        BeginDrawing();

            ClearBackground(DARKGRAY);

            BeginMode2D(camera);
                // DrawRectangleRec(rect,LIGHTGRAY);

                //draw player
                
                DrawLineEx(leftWing, playerPos, WING_THICKNESS, WHITE);
                DrawLineEx(playerPos, rightWing, WING_THICKNESS, WHITE);

                DrawCircleV(clickCircle, 20, RED);

                for(int i = 0; i < segmentCount; i++) {
                    DrawLineEx(segments[i].start, segments[i].end, SEGMENT_THICKNESS, RAYWHITE);
                }

                if(editMode) {

                switch(editModeCurrent) {
                    case EDIT_LINES_ADD:
                    break;
                    case EDIT_LINES_REMOVE:
                        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
                        for(int i = 0; i < segmentCount; i++) {
                            if(CollisionWithLine(mousePos, (Vector2){mousePos.x + 5, mousePos.y + 5}, (Vector2){mousePos.x - 5, mousePos.y - 5}, 
                                segments[i].start, segments[i].end)){
                                    DrawLineEx(segments[i].start, segments[i].end, SEGMENT_THICKNESS, YELLOW);
                                    DrawCircleV(segments[i].start, 20, YELLOW);
                                    DrawCircleV(segments[i].end, 20, YELLOW);
                            }
                        }
                    break;
                    case EDIT_GOAL_PLACE:
                    break;
                    default: break;
                }
            }

            EndMode2D();

            // Draw HUD text with debug info
            DrawText(TextFormat("Flap Velocity: %.2f", flapVelocity), 10, 10, 20, WHITE);
            DrawText(TextFormat("Flap Amount: %.2f", flapAmount), 10, 35, 20, WHITE);
            DrawText(TextFormat("Player Velocity: (%.2f, %.2f)", playerVelocity.x, playerVelocity.y), 10, 60, 20, WHITE);
            DrawText(TextFormat("Player Rotation: %.2f", playerRot), 10, 85, 20, WHITE);
            DrawText(TextFormat("Current Level: %d", currentLevel), 10, 110, 20, WHITE);
            DrawText(TextFormat("Edit Mode: %s", EditModeToString(editModeCurrent)), 10, 135, 20, WHITE);
            // DrawText(EditModeToString(editModeCurrent), 10, screenHeight - 30, 20, (editMode ? YELLOW : GREEN));
            
            // Draw graphs
            float graphX = screenWidth - GRAPH_WIDTH - 10;
            float graphY = 10;
            
            DrawGraph(flapVelocityHistory, GRAPH_SAMPLES, graphIndex, graphX, graphY, GRAPH_WIDTH, GRAPH_HEIGHT, RED, "Flap Velocity");
            DrawGraph(flapAmountHistory, GRAPH_SAMPLES, graphIndex, graphX, graphY + GRAPH_HEIGHT + 5, GRAPH_WIDTH, GRAPH_HEIGHT, BLUE, "Flap Amount");
            DrawGraph(playerVelocityMagnitudeHistory, GRAPH_SAMPLES, graphIndex, graphX, graphY + 2 * (GRAPH_HEIGHT + 5), GRAPH_WIDTH, GRAPH_HEIGHT, GREEN, "Player Speed");
            
            if(editMode) {
                DrawText("EDIT MODE - Press P to toggle", 10, screenHeight - 30, 20, YELLOW);
                DrawText("S to save segments", 10, screenHeight - 55, 20, YELLOW);
            } else {
                DrawText("PLAY MODE - Press P to toggle", 10, screenHeight - 30, 20, GREEN);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
