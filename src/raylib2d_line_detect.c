#include "raylib.h"
#include "raygui.h"

int main(void)
{
    // Initialize window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Mouse Point to Line Collision with Thickness Control");

    // Define line properties
    Vector2 lineStart = { 100, 100 };
    Vector2 lineEnd = { 700, 500 };
    float lineThickness = 5.0f; // Initial line thickness

    // GUI slider properties
    Rectangle sliderRect = { 50, 50, 200, 20 };
    float minThickness = 1.0f;
    float maxThickness = 20.0f;
    bool isDragging = false;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Update
        Vector2 mousePos = GetMousePosition();

        // Handle slider interaction
        if (CheckCollisionPointRec(mousePos, sliderRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            isDragging = true;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            isDragging = false;
        }
        if (isDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            // Update thickness based on mouse position within slider
            float sliderPos = mousePos.x - sliderRect.x;
            if (sliderPos < 0) sliderPos = 0;
            if (sliderPos > sliderRect.width) sliderPos = sliderRect.width;
            lineThickness = minThickness + (sliderPos / sliderRect.width) * (maxThickness - minThickness);
        }

        // Check collision between mouse and line
        bool collision = CheckCollisionPointLine(mousePos, lineStart, lineEnd, (int)lineThickness);

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw the line with specified thickness
        DrawLineEx(lineStart, lineEnd, lineThickness, collision ? RED : BLACK);

        // Draw mouse cursor as a small circle
        DrawCircleV(mousePos, 5.0f, collision ? RED : BLUE);

        // Draw GUI slider
        DrawRectangleRec(sliderRect, LIGHTGRAY);
        float sliderHandleX = sliderRect.x + ((lineThickness - minThickness) / (maxThickness - minThickness)) * sliderRect.width;
        DrawRectangle(sliderHandleX - 5, sliderRect.y - 5, 10, sliderRect.height + 10, DARKGRAY);

        // Draw thickness text
        DrawText(TextFormat("Line Thickness: %.1f", lineThickness), 50, 20, 20, BLACK);

        // Draw instructions
        DrawText("Click and drag slider to change line thickness", 50, 80, 20, DARKGRAY);
        DrawText("Move mouse to test collision with line", 50, 110, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}