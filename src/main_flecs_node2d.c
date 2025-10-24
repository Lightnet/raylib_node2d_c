// flecs v4.1.1
// raylib 5.5
// raygui 4.0

#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "flecs.h"

// #define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Initialize window
const int screenWidth = 800;
const int screenHeight = 600;

// PH
typedef enum {
    PIN_IN,
    PIN_OUT
} DIRECTION_PIN;

// PH
typedef enum {
    PIN_NONE,
    PIN_PLACE_HOLDER,
    PIN_FLOW,
    PIN_VARIABLE,
    PIN_TIME,
    PIN_TIMER,
    PIN_TRIGGER,
    PIN_FUNCTION
} CONNECTOR_PIN; // CONNECTOR_PIN is now an alias for the enum type

int pin_size = 10;
int pin_space = 8;

typedef struct {
  bool isMovementMode;
  bool tabPressed;
  bool moveForward;
  bool moveBackward;
  bool moveLeft;
  bool moveRight;
} player_input_t;
ECS_COMPONENT_DECLARE(player_input_t);

typedef struct {
    Vector2 pos;
    Vector2 off_set;
    bool drag;//resized?
    ecs_entity_t id;
    bool is_creating_connector; // New: Are we creating a connector?
    ecs_entity_t connector_start; // New: Start entity for connector
} mouse_t;
ECS_COMPONENT_DECLARE(mouse_t);

typedef struct {
    Camera3D camera_3d;
    Camera2D camera_2d;
} main_context_t;
ECS_COMPONENT_DECLARE(main_context_t);
//===============================================
//
//===============================================
typedef struct {
    Rectangle rect;
    // C-style string (char array)
    char text[256];
} text_t;
ECS_COMPONENT_DECLARE(text_t);

typedef struct {
    Rectangle rect;
} rect_t;
ECS_COMPONENT_DECLARE(rect_t);

// Rectangle
typedef struct {
    // Rectangle rect;
    // C-style string (char array)
    char text[256];
    bool edit_mode;
} text_box_t;
ECS_COMPONENT_DECLARE(text_box_t);

// GuiDummyRec
typedef struct {
    Rectangle rect;
    // C-style string (char array)
    char text[256];
} dummy_rect_t;
ECS_COMPONENT_DECLARE(dummy_rect_t);

//GuiColorPicker
typedef struct {
    // Rectangle rect;
    // C-style string (char array)
    // char text[256];
    Color picker;
} color_picker_t;
ECS_COMPONENT_DECLARE(color_picker_t);

/// @brief testing.
typedef struct {
    ecs_entity_t in;
    ecs_entity_t out;
} connector_t;
ECS_COMPONENT_DECLARE(connector_t);

// connector pin need position from the parent rect to adjust.
// required react
typedef struct {
    CONNECTOR_PIN pin;
    DIRECTION_PIN dir;
} connector_pin_t;
ECS_COMPONENT_DECLARE(connector_pin_t);

/*
//========//
[]  node  []
*/


//===============================================
// RENDER
//===============================================
// Render begin system
void RenderBeginSystem(ecs_iter_t *it) {
  // printf("RenderBeginSystem\n");
  BeginDrawing();
  ClearBackground(RAYWHITE);
}
// Render begin system
void BeginCamera3DSystem(ecs_iter_t *it) {
    // printf("BeginCamera3DSystem\n");
    main_context_t *main_context = ecs_field(it, main_context_t, 0); // Singleton
    if (!main_context) return;
    BeginMode3D(main_context->camera_3d);
}
// end camera 3d
void EndCamera3DSystem(ecs_iter_t *it) {
    //printf("EndCamera3DSystem\n");
    main_context_t *main_context = ecs_field(it, main_context_t, 0); // Singleton
    if (!main_context) return;
    EndMode3D();
}
// Render end system
void EndRenderSystem(ecs_iter_t *it) {
  //printf("EndRenderSystem\n");
  EndDrawing();
}

// Update BeginCamera3DSystem:
void BeginCamera2DSystem(ecs_iter_t *it) {
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    if (main_context) BeginMode2D(main_context->camera_2d);
}
// Update EndCamera3DSystem:
void EndCamera2DSystem(ecs_iter_t *it) {
    main_context_t *main_context = ecs_field(it, main_context_t, 0);
    if (main_context) EndMode2D();
}


//===============================================
// DRAW
//===============================================

// Grid settings
const int gridSize = 8; // Size of each grid cell in pixels
Color gridColor = { 200, 200, 200, 255 }; // Light gray color

void render2d_grid_system(ecs_iter_t *it){
    // DrawText("Test", 0, 0, 20, DARKGRAY);
    // Draw vertical lines
    for (int x = 0; x <= screenWidth; x += gridSize)
    {
        DrawLine(x, 0, x, screenHeight, gridColor);
    }

    // Draw horizontal lines
    for (int y = 0; y <= screenHeight; y += gridSize)
    {
        DrawLine(0, y, screenWidth, y, gridColor);
    }
}

//===============================================
// DRAW TEXT
//===============================================
// draw text
void render2d_text_system(ecs_iter_t *it){
    rect_t *rect = ecs_field(it, rect_t, 0);
    text_t *text2d = ecs_field(it, text_t, 1);
    for (int i = 0; i < it->count; i++) {
        // ecs_entity_t entity = transform3d[i].id;
        // ecs_entity_t entity = it->entities[i];
        DrawText(text2d[i].text, rect[i].rect.x, rect[i].rect.y, 20, DARKGRAY);
    }
}
// rect text test
// void render2d_text_select_system(ecs_iter_t *it){
//     rect_t *rect = ecs_field(it, rect_t, 0);
//     text_t *text2d = ecs_field(it, text_t, 1);
//     for (int i = 0; i < it->count; i++) {
//         // ecs_entity_t entity = transform3d[i].id;
//         ecs_entity_t entity = it->entities[i];
//         //DrawText(text2d[i].text, text2d[i].rect.x, text2d->rect.y, 20, DARKGRAY);
//         // bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
//         // Vector2 GetMousePosition(void);
//         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
//             if(CheckCollisionPointRec( GetMousePosition(),rect[i].rect)){
//                 printf("found click\n");
//             }
//         }
//         DrawRectangleLines(rect[i].rect.x,rect[i].rect.y,rect[i].rect.width,rect[i].rect.height, GRAY);
//     }
// }
// text (drag mouse)
void render2d_text_drag_system(ecs_iter_t *it) {
    mouse_t *mouse = ecs_singleton_get_mut(it->world, mouse_t);
    rect_t *rect = ecs_field(it, rect_t, 0);
    text_t *text = ecs_field(it, text_t, 1);

    // Get current mouse position
    Vector2 mouse_pos = GetMousePosition();

    // If a textbox is being dragged (mouse->id is set)
    if (mouse->id) {
        // Check if the mouse button is still held
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // Update the textbox position based on mouse position and offset
            rect_t *rect = ecs_get_mut(it->world, mouse->id, rect_t);
            // text_box_t *selected_textbox = ecs_get_mut(it->world, mouse->id, text_box_t);

            // if (selected_textbox) {
                rect->rect.x = mouse_pos.x - mouse->off_set.x;
                rect->rect.y = mouse_pos.y - mouse->off_set.y;
                DrawRectangleLines(rect->rect.x,rect->rect.y,rect->rect.width,rect->rect.height, GREEN);
                // ecs_modified(it->world, mouse->id, text_box_t); // Notify ECS of the change
            // }
        } else {
            // Mouse button released, end drag
            mouse->id = 0;
            mouse->off_set = (Vector2){0, 0};
            ecs_singleton_modified(it->world, mouse_t); // Notify ECS of mouse_t change
        }
    } else {
        // No textbox is being dragged, check for new click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < it->count; i++) {
                ecs_entity_t entity = it->entities[i];
                if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
                    // Start dragging this textbox
                    mouse->id = entity;
                    // Calculate offset from mouse to textbox top-left
                    mouse->off_set.x = mouse_pos.x - rect[i].rect.x;
                    mouse->off_set.y = mouse_pos.y - rect[i].rect.y;
                    printf("Started dragging entity %llu\n", (unsigned long long)entity);
                    ecs_singleton_modified(it->world, mouse_t); // Notify ECS of mouse_t change
                    break; // Only drag one textbox at a time
                }
            }
        }

        for (int i = 0; i < it->count; i++) {
            ecs_entity_t entity = it->entities[i];
            if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
                // Start draw this text rectangle
                DrawRectangleLines(rect[i].rect.x,rect[i].rect.y,rect[i].rect.width,rect[i].rect.height, GRAY);
                break; // Only drag one textbox at a time
            }
        }
    }
}

//===============================================
// TEXT BOX
//===============================================
// text box (input text)
void render2d_text_box_system(ecs_iter_t *it){
    rect_t *rect = ecs_field(it, rect_t, 0);
    text_box_t *textbox2d = ecs_field(it, text_box_t, 1);
    for (int i = 0; i < it->count; i++) {
        // ecs_entity_t entity = transform3d[i].id;
        // ecs_entity_t entity = it->entities[i];
        if(GuiTextBox(rect[i].rect, textbox2d[i].text, 20, textbox2d[i].edit_mode)) textbox2d[i].edit_mode=!textbox2d[i].edit_mode;
    }
}
// text box (drag mouse)
void render2d_text_box_drag_system(ecs_iter_t *it) {
    mouse_t *mouse = ecs_singleton_get_mut(it->world, mouse_t);
    rect_t *rect = ecs_field(it, rect_t, 0);
    text_box_t *text_box = ecs_field(it, text_box_t, 1);

    // Get current mouse position
    Vector2 mouse_pos = GetMousePosition();

    // If a textbox is being dragged (mouse->id is set)
    if (mouse->id) {
        // Check if the mouse button is still held
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // Update the textbox position based on mouse position and offset
            rect_t *rect = ecs_get_mut(it->world, mouse->id, rect_t);
            text_box_t *selected_textbox = ecs_get_mut(it->world, mouse->id, text_box_t);

            if (selected_textbox) {
                rect->rect.x = mouse_pos.x - mouse->off_set.x;
                rect->rect.y = mouse_pos.y - mouse->off_set.y;
                ecs_modified(it->world, mouse->id, text_box_t); // Notify ECS of the change
            }
        } else {
            // Mouse button released, end drag
            mouse->id = 0;
            mouse->off_set = (Vector2){0, 0};
            ecs_singleton_modified(it->world, mouse_t); // Notify ECS of mouse_t change
        }
    } else {
        // No textbox is being dragged, check for new click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < it->count; i++) {
                ecs_entity_t entity = it->entities[i];
                if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
                    // Start dragging this textbox
                    mouse->id = entity;
                    // Calculate offset from mouse to textbox top-left
                    mouse->off_set.x = mouse_pos.x - rect[i].rect.x;
                    mouse->off_set.y = mouse_pos.y - rect[i].rect.y;
                    printf("Started dragging entity %llu\n", (unsigned long long)entity);
                    ecs_singleton_modified(it->world, mouse_t); // Notify ECS of mouse_t change
                    break; // Only drag one textbox at a time
                }
            }
        }
    }
}

// check test
// void render2d_text_box_select_system(ecs_iter_t *it){
//     text_t *text2d = ecs_field(it, text_t, 0);
//     for (int i = 0; i < it->count; i++) {
//         // ecs_entity_t entity = transform3d[i].id;
//         ecs_entity_t entity = it->entities[i];
//         //DrawText(text2d[i].text, text2d[i].rect.x, text2d->rect.y, 20, DARKGRAY);
//         // bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
//         // Vector2 GetMousePosition(void);
//         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
//             if(CheckCollisionPointRec( GetMousePosition(),text2d[i].rect)){
//                 printf("found click\n");
//             }
//         }
//         DrawRectangleLines(text2d[i].rect.x,text2d[i].rect.y,text2d[i].rect.width,text2d[i].rect.height, GRAY);
//     }
// }
//===============================================
// TEXT BOX
//===============================================

//===============================================
// CONNECTORS
//===============================================
void render2d_connector_system(ecs_iter_t *it) {
    connector_t *connector = ecs_field(it, connector_t, 0);
    
    for (int i = 0; i < it->count; i++) {
        // Get the rect_t components of the in and out entities
        rect_t *in_rect = ecs_get_mut(it->world, connector[i].in, rect_t);
        rect_t *out_rect = ecs_get_mut(it->world, connector[i].out, rect_t);
        
        if (in_rect && out_rect) {
            // Calculate the center points of the rectangles
            Vector2 start = (Vector2){
                in_rect->rect.x + in_rect->rect.width / 2,
                in_rect->rect.y + in_rect->rect.height / 2
            };
            Vector2 end = (Vector2){
                out_rect->rect.x + out_rect->rect.width / 2,
                out_rect->rect.y + out_rect->rect.height / 2
            };
            // Draw a line between the centers
            DrawLineV(start, end, BLACK);
        }
    }
}

// test
// void connector_creation_system(ecs_iter_t *it) {
//     mouse_t *mouse = ecs_singleton_get_mut(it->world, mouse_t);
//     rect_t *rect = ecs_field(it, rect_t, 0);
//     Vector2 mouse_pos = GetMousePosition();
//     // Start connector creation with 'C' key
//     if (IsKeyPressed(KEY_C)) {
//         mouse->is_creating_connector = true;
//         mouse->connector_start = 0;
//         ecs_singleton_modified(it->world, mouse_t);
//     }
//     // Handle mouse clicks during connector creation
//     if (mouse->is_creating_connector && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
//         for (int i = 0; i < it->count; i++) {
//             ecs_entity_t entity = it->entities[i];
//             if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
//                 if (mouse->connector_start == 0) {
//                     // First node selected
//                     mouse->connector_start = entity;
//                     printf("Selected start node %llu\n", (unsigned long long)entity);
//                 } else if (mouse->connector_start != entity) {
//                     // Second node selected, create connector
//                     ecs_entity_t connector = ecs_new(it->world);
//                     ecs_set(it->world, connector, connector_t, {
//                         .in = mouse->connector_start,
//                         .out = entity
//                     });
//                     printf("Created connector from %llu to %llu\n",
//                            (unsigned long long)mouse->connector_start,
//                            (unsigned long long)entity);
//                     // Reset connector creation state
//                     mouse->is_creating_connector = false;
//                     mouse->connector_start = 0;
//                 }
//                 ecs_singleton_modified(it->world, mouse_t);
//                 break;
//             }
//         }
//     }
//     // Draw preview line during connector creation
//     if (mouse->is_creating_connector && mouse->connector_start != 0) {
//         rect_t *start_rect = ecs_get_mut(it->world, mouse->connector_start, rect_t);
//         if (start_rect) {
//             Vector2 start = (Vector2){
//                 start_rect->rect.x + start_rect->rect.width / 2,
//                 start_rect->rect.y + start_rect->rect.height / 2
//             };
//             DrawLineV(start, mouse_pos, BLUE); // Preview line
//         }
//     }
// }

void render_2d_draw_pins_system(ecs_iter_t *it){
    rect_t *rect = ecs_field(it, rect_t, 0);
    connector_pin_t *connector_pin = ecs_field(it, connector_pin_t, 1);

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t entity = it->entities[i];
        DrawRectangleLines(rect[i].rect.x,rect[i].rect.y,rect[i].rect.width,rect[i].rect.height, ORANGE);
    }
}
//
void connector_pin_creation_system(ecs_iter_t *it) {
    mouse_t *mouse = ecs_singleton_get_mut(it->world, mouse_t);
    rect_t *rect = ecs_field(it, rect_t, 0);
    connector_pin_t *connector_pin = ecs_field(it, connector_pin_t, 1);
    
    Vector2 mouse_pos = GetMousePosition();

    // Start connector creation with 'C' key
    if (IsKeyPressed(KEY_C)) {
        // if (mouse->is_creating_connector)
        mouse->is_creating_connector = true;
        mouse->connector_start = 0;
        // ecs_singleton_modified(it->world, mouse_t);
    }

    // Handle mouse clicks during connector creation
    if (mouse->is_creating_connector && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool is_found = false;
        for (int i = 0; i < it->count; i++) {
            ecs_entity_t entity = it->entities[i];
            if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
                if (mouse->connector_start == 0) {
                    // First node selected
                    mouse->connector_start = entity;
                    printf("Selected start node %llu\n", (unsigned long long)entity);
                    is_found = true;
                } else if (mouse->connector_start != entity) {
                    is_found = true;
                    // Second node selected, create connector
                    ecs_entity_t connector = ecs_new(it->world);
                    ecs_set(it->world, connector, connector_t, {
                        .in = mouse->connector_start,
                        .out = entity
                    });
                    printf("Created connector from %llu to %llu\n",
                           (unsigned long long)mouse->connector_start,
                           (unsigned long long)entity);
                    // Reset connector creation state
                    mouse->is_creating_connector = false;
                    mouse->connector_start = 0;
                }
                // ecs_singleton_modified(it->world, mouse_t);
                break;
            }
        }
        if(is_found == false){//not rect area close it.
            mouse->is_creating_connector = false;
            mouse->connector_start = 0;
        }
    }

    // Draw preview line during connector creation
    if (mouse->is_creating_connector && mouse->connector_start != 0) {
        rect_t *start_rect = ecs_get_mut(it->world, mouse->connector_start, rect_t);
        if (start_rect) {
            Vector2 start = (Vector2){
                start_rect->rect.x + start_rect->rect.width / 2,
                start_rect->rect.y + start_rect->rect.height / 2
            };
            DrawLineV(start, mouse_pos, BLUE); // Preview line
        }
    }

}





//===============================================
//
//===============================================
// main
int main(void) {
    InitWindow(800, 600, "Node 2D Flecs v4.1");
    SetTargetFPS(60);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, player_input_t);
    ECS_COMPONENT_DEFINE(world, main_context_t);
    ECS_COMPONENT_DEFINE(world, mouse_t);
    ECS_COMPONENT_DEFINE(world, text_t);
    ECS_COMPONENT_DEFINE(world, rect_t);
    ECS_COMPONENT_DEFINE(world, text_box_t);

    ECS_COMPONENT_DEFINE(world, connector_t);
    ECS_COMPONENT_DEFINE(world, connector_pin_t);
    

    // Define custom phases
    ecs_entity_t PreLogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t BeginRenderPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t BeginCamera3DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t Camera3DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t EndCamera3DPhase = ecs_new_w_id(world, EcsPhase);

    ecs_entity_t BeginCamera2DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t Camera2DPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t EndCamera2DPhase = ecs_new_w_id(world, EcsPhase);

    ecs_entity_t RenderPhase = ecs_new_w_id(world, EcsPhase);
    ecs_entity_t EndRenderPhase = ecs_new_w_id(world, EcsPhase);

    // order phase must be correct order
    ecs_add_pair(world, PreLogicUpdatePhase, EcsDependsOn, EcsPreUpdate); // start game logics
    ecs_add_pair(world, LogicUpdatePhase, EcsDependsOn, PreLogicUpdatePhase); // start game logics
    ecs_add_pair(world, BeginRenderPhase, EcsDependsOn, LogicUpdatePhase); // BeginDrawing

    ecs_add_pair(world, BeginCamera3DPhase, EcsDependsOn, BeginRenderPhase); // EcsOnUpdate, BeginMode3D
    ecs_add_pair(world, Camera3DPhase, EcsDependsOn, BeginCamera3DPhase); // 3d model only
    ecs_add_pair(world, EndCamera3DPhase, EcsDependsOn, Camera3DPhase); // EndMode3D

    ecs_add_pair(world, BeginCamera2DPhase, EcsDependsOn, EndCamera3DPhase); // EcsOnUpdate, BeginMode2D
    ecs_add_pair(world, Camera2DPhase, EcsDependsOn, BeginCamera2DPhase); // 2D model only
    ecs_add_pair(world, EndCamera2DPhase, EcsDependsOn, Camera2DPhase); // EndMode2D

    ecs_add_pair(world, RenderPhase, EcsDependsOn, EndCamera2DPhase); // 2D only
    ecs_add_pair(world, EndRenderPhase, EcsDependsOn, RenderPhase); // render to screen

    // note this has be in order of the ECS since push into array. From I guess.
    // Render
    ecs_system(world,{
      .entity = ecs_entity(world, { .name = "RenderBeginSystem", 
        .add = ecs_ids(ecs_dependson(BeginRenderPhase)) 
      }),
      .callback = RenderBeginSystem
    });
    // begin camera 3d
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "BeginCamera3DSystem", 
            .add = ecs_ids(ecs_dependson(BeginCamera3DPhase)) 
        }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) } // Singleton
        },
        .callback = BeginCamera3DSystem
    });
    // end camera 3d
    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "EndCamera3DSystem", 
            .add = ecs_ids(ecs_dependson(EndCamera3DPhase))
        }),
        .query.terms = {
            { .id = ecs_id(main_context_t), .src.id = ecs_id(main_context_t) } // Singleton
        },
        .callback = EndCamera3DSystem
    });
    // end render
    ecs_system(world, {
        .entity = ecs_entity(world, {
          .name = "EndRenderSystem", 
          .add = ecs_ids(ecs_dependson(EndRenderPhase)) 
        }),
        .callback = EndRenderSystem
    });
    // render 2d, grid
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_grid_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .callback = render2d_grid_system
    });
    // render 2d, debug
    // ecs_system(world, {
    //   .entity = ecs_entity(world, { .name = "render2d_debug_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
    // //   .query.terms = {
    // //     { .id = ecs_id(player_input_t), .src.id = ecs_id(player_input_t) } // Singleton
    // //   },
    //   .callback = render2d_debug_system
    // });
    // render 2d, text
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_t)} // 
      },
      .callback = render2d_text_system
    });
    // render 2d, text select
    // ecs_system(world, {
    //   .entity = ecs_entity(world, { .name = "render2d_text_select_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
    //   .query.terms = {
    //     { .id = ecs_id(rect_t)}, // 
    //     { .id = ecs_id(text_t)} // 
    //   },
    //   .callback = render2d_text_select_system
    // });

    // render 2d, text drag
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_drag_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_t)} // 
      },
      .callback = render2d_text_drag_system
    });


    // render 2d, text box
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_box_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_box_t)} // 
      },
      .callback = render2d_text_box_system
    });
    // render 2d, text box, move
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_box_drag_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_box_t)} // 
      },
      .callback = render2d_text_box_drag_system
    });

//===============================================
// CONNECTORS
//===============================================

    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render2d_connector_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
        .query.terms = {
            { .id = ecs_id(connector_t) }
        },
        .callback = render2d_connector_system
    });

    // ecs_system(world, {
    //     .entity = ecs_entity(world, { .name = "connector_creation_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
    //     .query.terms = {
    //         { .id = ecs_id(rect_t) }
    //     },
    //     .callback = connector_creation_system
    // });


    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_2d_draw_pins_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(rect_t) },
            { .id = ecs_id(connector_pin_t) }
        },
        .callback = render_2d_draw_pins_system
    });

    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "connector_pin_creation_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
        .query.terms = {
            { .id = ecs_id(rect_t) },
            { .id = ecs_id(connector_pin_t) },
        },
        .callback = connector_pin_creation_system
    });


//===============================================
// 
//===============================================
    // camera
    Camera3D camera_3d = {
        .position = (Vector3){10.0f, 10.0f, 10.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    Camera2D camera_2d = {
        .offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f},
        .target = (Vector2){0.0f, 0.0f},
        .rotation = 0.0f,
        .zoom = 1.0f
    };
    ecs_singleton_set(world, main_context_t, {
        .camera_3d = camera_3d,
        .camera_2d = camera_2d
    });
    // mouse
    ecs_singleton_set(world, mouse_t, {
        .pos = {0, 0},
        .off_set = {0, 0},
        .drag = false,
        .id = 0,
        .is_creating_connector = false,
        .connector_start = 0
    });
//===============================================
// 
//===============================================

    ecs_entity_t text1 = ecs_new(world);
    ecs_set(world, text1, rect_t, {
        .rect = (Rectangle){0,20,120,24}
    });
    ecs_set(world, text1, text_t, {
        .text = "flecs test!"
    });



    ecs_entity_t textbox1 = ecs_new(world);
    ecs_set(world, textbox1, rect_t, {
        .rect = (Rectangle){0,50,120,24},
    });
    ecs_set(world, textbox1, text_box_t, {
        // .rect = (Rectangle){0,50,120,24},
        .text = "flecs test!",
        .edit_mode = true
    });

    ecs_entity_t textbox2 = ecs_new(world);
    ecs_set(world, textbox2, rect_t, {
        .rect = (Rectangle){0,80,120,24},
    });
    ecs_set(world, textbox2, text_box_t, {
        .text = "flecs test!2",
        .edit_mode = true
    });


    ecs_entity_t connector1 = ecs_new(world);
    ecs_set(world, connector1, connector_t, {
        .in = text1,
        .out = textbox1
    });


    ecs_entity_t connector_pin1 = ecs_new(world);
    ecs_set(world, connector_pin1, rect_t, {
        .rect = (Rectangle)
        {
            10,
            100,
            pin_size,
            pin_size
        }
    });
    ecs_set(world, connector_pin1, connector_pin_t, {
        .pin = PIN_PLACE_HOLDER,
        .dir = PIN_IN
    });

    ecs_entity_t connector_pin2 = ecs_new(world);
    ecs_set(world, connector_pin2, rect_t, {
        .rect = (Rectangle)
        {
            10,
            200,
            pin_size,
            pin_size
        }
    });
    ecs_set(world, connector_pin2, connector_pin_t, {
        .pin = PIN_PLACE_HOLDER,
        .dir = PIN_IN
    });

    ecs_entity_t connector_pin3 = ecs_new(world);
    ecs_set(world, connector_pin3, rect_t, {
        .rect = (Rectangle)
        {
            150,
            250,
            pin_size,
            pin_size
        }
    });
    ecs_set(world, connector_pin3, connector_pin_t, {
        .pin = PIN_PLACE_HOLDER,
        .dir = PIN_IN
    });

//===============================================
// 
//===============================================

    while (!WindowShouldClose()) {
      ecs_progress(world, 0);
    }

    // UnloadModel(cube);
    ecs_fini(world);
    CloseWindow();
    return 0;
}