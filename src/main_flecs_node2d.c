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
//===============================================
//
//===============================================

// Define components (unchanged)
typedef struct {
    Vector2 local_pos;    // Local position relative to parent
    Vector2 world_pos;    // Computed world position
    Vector2 local_scale;  // Local scale relative to parent
    float local_rotation; // Local rotation in degrees
    float world_rotation; // Computed world rotation in degrees
    bool isDirty;        // Flag to indicate if transform needs updating
} transform_2d_t;
ECS_COMPONENT_DECLARE(transform_2d_t);

typedef struct {
    ecs_entity_t id;
} transform_2d_select_t;
ECS_COMPONENT_DECLARE(transform_2d_select_t);

//===============================================
//
//===============================================

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

/// @brief this for enable node for pins connectors
typedef struct {
    bool enable;
} node_2d_t;
ECS_COMPONENT_DECLARE(node_2d_t);
//===============================================
// Transform 3D
//===============================================

float NormalizeAngle(float degrees) {
    degrees = fmodf(degrees, 360.0f);
    if (degrees > 180.0f) {
        degrees -= 360.0f;
    } else if (degrees < -180.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

// Helper function to rotate a Vector2 around the origin by an angle (in degrees)
Vector2 RotateVector2(Vector2 v, float degrees) {
    float radians = DEG2RAD * degrees;
    float cos_r = cosf(radians);
    float sin_r = sinf(radians);
    return (Vector2){
        v.x * cos_r - v.y * sin_r,
        v.x * sin_r + v.y * cos_r
    };
}

// Helper function to update a single transform
void UpdateTransform2D(ecs_world_t *world, ecs_entity_t entity, transform_2d_t *transform) {
    // Check if parent is dirty
    ecs_entity_t parent = ecs_get_parent(world, entity);
    bool parentIsDirty = false;
    if (parent && ecs_is_valid(world, parent)) {
        const transform_2d_t *parent_transform = ecs_get(world, parent, transform_2d_t);
        if (parent_transform && parent_transform->isDirty) {
            parentIsDirty = true;
        }
    }

    // Skip update if neither this transform nor its parent is dirty
    if (!transform->isDirty && !parentIsDirty) {
        return;
    }

    // Compute world transform
    if (!parent || !ecs_is_valid(world, parent)) {
        // Root entity: world = local
        transform->world_pos = transform->local_pos;
        transform->world_rotation = NormalizeAngle(transform->local_rotation); // Normalize
    } else {
        // Child entity: compute world position and rotation
        const transform_2d_t *parent_transform = ecs_get(world, parent, transform_2d_t);
        if (!parent_transform) {
            transform->world_pos = transform->local_pos;
            transform->world_rotation = NormalizeAngle(transform->local_rotation); // Normalize
            return;
        }

        // Validate parent world position
        if (fabs(parent_transform->world_pos.x) > 1e6 || fabs(parent_transform->world_pos.y) > 1e6) {
            transform->world_pos = transform->local_pos;
            transform->world_rotation = NormalizeAngle(transform->local_rotation); // Normalize
            return;
        }

        // Rotate local_pos around origin by parent's world_rotation, then translate by parent's world_pos
        transform->world_pos = RotateVector2(transform->local_pos, parent_transform->world_rotation);
        transform->world_pos = Vector2Add(transform->world_pos, parent_transform->world_pos);
        // Combine rotations and normalize
        transform->world_rotation = NormalizeAngle(transform->local_rotation + parent_transform->world_rotation);
    }

    // Mark children as dirty
    ecs_iter_t it = ecs_children(world, entity);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            transform_2d_t *child_transform = ecs_get_mut(world, it.entities[i], transform_2d_t);
            if (child_transform) {
                child_transform->isDirty = true;
                ecs_modified(world, it.entities[i], transform_2d_t);
            }
        }
    }

    // Reset isDirty
    transform->isDirty = false;
}

// Function to update a single entity and its descendants
void UpdateChildTransform2DOnly(ecs_world_t *world, ecs_entity_t entity) {
    transform_2d_t *transform = ecs_get_mut(world, entity, transform_2d_t);
    if (!transform) return;

    UpdateTransform2D(world, entity, transform);
    ecs_modified(world, entity, transform_2d_t);

    ecs_iter_t it = ecs_children(world, entity);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            UpdateChildTransform2DOnly(world, it.entities[i]);
        }
    }
}

// System to process all entities with transform_2d_t
void update_transform_2d_system(ecs_iter_t *it) {
    transform_2d_t *transforms = ecs_field(it, transform_2d_t, 0);

    // Update root entity's transform
    // for testing
    // for (int i = 0; i < it->count; i++) {
    //     ecs_entity_t current_transform = it->entities[i];
    //     if (!ecs_is_valid(it->world, current_transform)) {
    //         continue;
    //     }
    //     transform_2d_t *transform = &transforms[i];
    //     if (!ecs_get_parent(it->world, current_transform)) {
    //         // transform->local_rotation = 60.0f * GetTime(); // Rotate at 60 degrees per second
    //         transform->local_rotation = NormalizeAngle(60.0f * GetTime()); // Rotate at 60 degrees per second, normalized
    //         transform->local_pos.x = 400 + 100 * sinf((float)GetTime()); // Oscillate horizontally
    //         transform->local_pos.y = 300;
    //         transform->isDirty = true;
    //     }
    // }

    // Update all entities hierarchically
    for (int i = 0; i < it->count; i++) {
        ecs_entity_t current_transform = it->entities[i];
        if (!ecs_is_valid(it->world, current_transform)) {
            continue;
        }
        UpdateChildTransform2DOnly(it->world, current_transform);
    }
}

void update_transform_2d_rect_system(ecs_iter_t *it){
    transform_2d_t *transforms = ecs_field(it, transform_2d_t, 0);
    rect_t *rect = ecs_field(it, rect_t, 1);

    for (int i = 0; i < it->count; i++) {
       rect[i].rect.x = transforms[i].world_pos.x;
       rect[i].rect.y = transforms[i].world_pos.y;
    }
}

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

// text (drag mouse)
void render2d_text_drag_system(ecs_iter_t *it) {
    mouse_t *mouse = ecs_singleton_get_mut(it->world, mouse_t);
    rect_t *rect = ecs_field(it, rect_t, 0);
    text_t *text = ecs_field(it, text_t, 1);
    transform_2d_t *transform = ecs_field(it, transform_2d_t, 2);

    // Get current mouse position
    Vector2 mouse_pos = GetMousePosition();

    // Handle dragging
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (mouse->id == 0) {
            // No entity is being dragged, check for a new selection
            for (int i = 0; i < it->count; i++) {
                ecs_entity_t entity = it->entities[i];
                if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
                    // Start dragging this entity
                    mouse->id = entity;
                    mouse->off_set.x = mouse_pos.x - transform[i].local_pos.x;
                    mouse->off_set.y = mouse_pos.y - transform[i].local_pos.y;
                    printf("Started dragging entity %llu\n", (unsigned long long)entity);
                    ecs_singleton_modified(it->world, mouse_t);
                    break;
                }
            }
        } else {
            // An entity is being dragged, update its position
            for (int i = 0; i < it->count; i++) {
                if (it->entities[i] == mouse->id) {
                    transform[i].local_pos.x = mouse_pos.x - mouse->off_set.x;
                    transform[i].local_pos.y = mouse_pos.y - mouse->off_set.y;
                    transform[i].isDirty = true;
                    printf("pos x: %f y:%f\n", transform[i].world_pos.x, transform[i].world_pos.x);
                    ecs_modified(it->world, it->entities[i], transform_2d_t);
                    break;
                }
            }
        }
    } else if (mouse->id != 0) {
        // Mouse button released, stop dragging
        mouse->id = 0;
        mouse->off_set = (Vector2){0, 0};
        ecs_singleton_modified(it->world, mouse_t);
    }

    // Draw outlines for hover and dragging feedback
    for (int i = 0; i < it->count; i++) {
        Color outline_color = (it->entities[i] == mouse->id) ? GREEN : GRAY;
        if (CheckCollisionPointRec(mouse_pos, rect[i].rect)) {
            DrawRectangleLines(rect[i].rect.x, rect[i].rect.y, rect[i].rect.width, rect[i].rect.height, outline_color);
        }
        
        DrawRectangleLines(transform[i].world_pos.x, transform[i].world_pos.y, rect[i].rect.width, rect[i].rect.height, outline_color);
    }
}


//===============================================
// TEXT BOX
//===============================================
// text box (input text)
void render2d_text_box_system(ecs_iter_t *it){
    rect_t *rect = ecs_field(it, rect_t, 0);
    text_box_t *textbox2d = ecs_field(it, text_box_t, 1);
    transform_2d_t *transform_2d = ecs_field(it, transform_2d_t, 2);
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
    transform_2d_t *transform = ecs_field(it, transform_2d_t, 2);

    // Get current mouse position
    Vector2 mouse_pos = GetMousePosition();

    // If a textbox is being dragged (mouse->id is set)
    if (mouse->id) {
        printf("id: %d\n", mouse->id);
        // Check if the mouse button is still held
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // Update the textbox position based on mouse position and offset
            // rect_t *select_rect = ecs_get_mut(it->world, mouse->id, rect_t);
            // text_box_t *selected_textbox = ecs_get_mut(it->world, mouse->id, text_box_t);
            transform_2d_t *transform_2d = ecs_get_mut(it->world, mouse->id, transform_2d_t);

            // if (selected_textbox) {
                transform_2d->local_pos.x = mouse_pos.x - mouse->off_set.x;
                transform_2d->local_pos.y = mouse_pos.y - mouse->off_set.y;
                transform_2d->isDirty = true;
                rect->rect.x = transform_2d->local_pos.x;
                rect->rect.y = transform_2d->local_pos.y;
                // ecs_modified(it->world, mouse->id, text_box_t); // Notify ECS of the change
            // }
        } else {
            // Mouse button released, end drag
            mouse->id = 0;
            mouse->off_set = (Vector2){0, 0};
            // ecs_singleton_modified(it->world, mouse_t); // Notify ECS of mouse_t change
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

// draw pins
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
// display connector
void render_2d_connector_status_system(ecs_iter_t *it) {
    // mouse_t *mouse = ecs_singleton_get_mut(it->world, mouse_t);
    // connector_pin_t *connector_pin = ecs_field(it, connector_pin_t, 1);
    mouse_t *mouse = ecs_field(it, mouse_t, 0);
    DrawText(TextFormat("Is Connector:  %s",mouse->is_creating_connector ? "TRUE" : "FALSE"),0,0,20,GRAY);
    // printf("render\n");
}


//===============================================
// 
//===============================================
void render_2d_transform_list_system(ecs_iter_t *it){
    // Singleton
    transform_2d_select_t *transform_2d_select = ecs_field(it, transform_2d_select_t, 0);  // Field index 0
    // transform_2d_t *transform_2d = ecs_field(it, transform_2d_t, 1);

    // Create a query for all entities with transform_2d_t
    ecs_query_t *query = ecs_query(it->world, {
        .terms = {{ ecs_id(transform_2d_t) }}
    });

    int entity_count = 0;
    ecs_iter_t transform_it = ecs_query_iter(it->world, query);
    while (ecs_query_next(&transform_it)) {
        entity_count += transform_it.count;
    }

    // Allocate buffers for entity names and IDs
    ecs_entity_t *entity_ids = (ecs_entity_t *)RL_MALLOC(entity_count * sizeof(ecs_entity_t));
    char **entity_names = (char **)RL_MALLOC(entity_count * sizeof(char *));
    int index = 0;

    // Populate entity names and IDs
    transform_it = ecs_query_iter(it->world, query);
    while (ecs_query_next(&transform_it)) {
        for (int j = 0; j < transform_it.count; j++) {
            const char *name = ecs_get_name(it->world, transform_it.entities[j]);
            entity_names[index] = (char *)RL_MALLOC(256 * sizeof(char));
            snprintf(entity_names[index], 256, "%s", name ? name : "(unnamed)");
            entity_ids[index] = transform_it.entities[j];
            index++;
        }
    }

    // Create a single string for GuiListView
    char *name_list = (char *)RL_MALLOC(entity_count * 256 * sizeof(char));
    name_list[0] = '\0';
    for (int j = 0; j < entity_count; j++) {
        if (j > 0) strcat(name_list, ";");
        strcat(name_list, entity_names[j]);
    }

    // Draw the list view on the right side
    Rectangle list_rect = {520, 30, 240, 200};
    static int scroll_index = 0;
    static int selected_index = -1;
    // border
    Rectangle panel_rect = list_rect;
    panel_rect.x -= 8;
    panel_rect.y -= 8;
    panel_rect.width += 16;
    panel_rect.height += 10;


    GuiGroupBox(panel_rect, "Entity List");

    GuiListView(list_rect, name_list, &scroll_index, &selected_index);

    // Draw transform controls if an entity is selected
    if (selected_index >= 0 && selected_index < entity_count && ecs_is_valid(it->world, entity_ids[selected_index])) {
        transform_2d_select->id = entity_ids[selected_index];
        transform_2d_t *transform = ecs_get_mut(it->world, transform_2d_select->id, transform_2d_t);
        bool modified = false;

        if (transform) {
            Rectangle control_rect = {510, 240, 260, 200};
            GuiGroupBox(control_rect, "Transform Controls");

            // Position controls
            GuiLabel((Rectangle){530, 250, 100, 20}, "Position");
            float new_pos_x = transform->local_pos.x;
            float new_pos_y = transform->local_pos.y;
            GuiSlider((Rectangle){530, 270, 200, 20}, "X", TextFormat("%.2f", new_pos_x), &new_pos_x, -800.0f, 800.0f);
            GuiSlider((Rectangle){530, 290, 200, 20}, "Y", TextFormat("%.2f", new_pos_y), &new_pos_y, -600.0f, 600.0f);
            if (new_pos_x != transform->local_pos.x || new_pos_y != transform->local_pos.y) {
                transform->local_pos.x = new_pos_x;
                transform->local_pos.y = new_pos_y;
                modified = true;
            }

            // Rotation controls
            GuiLabel((Rectangle){530, 310, 100, 20}, "Rotation");
            float new_rotation = transform->local_rotation;
            GuiSlider((Rectangle){530, 330, 200, 20}, "Angle", TextFormat("%.2f", new_rotation), &new_rotation, -180.0f, 180.0f);
            if (new_rotation != transform->local_rotation) {
                // transform->local_rotation = new_rotation;
                transform->local_rotation = NormalizeAngle(new_rotation); // Normalize
                modified = true;
            }

            // Scale controls
            GuiLabel((Rectangle){530, 350, 100, 20}, "Scale");
            float new_scale_x = transform->local_scale.x;
            float new_scale_y = transform->local_scale.y;
            GuiSlider((Rectangle){530, 370, 200, 20}, "X", TextFormat("%.2f", new_scale_x), &new_scale_x, 0.1f, 5.0f);
            GuiSlider((Rectangle){530, 390, 200, 20}, "Y", TextFormat("%.2f", new_scale_y), &new_scale_y, 0.1f, 5.0f);
            if (new_scale_x != transform->local_scale.x || new_scale_y != transform->local_scale.y) {
                transform->local_scale.x = new_scale_x;
                transform->local_scale.y = new_scale_y;
                modified = true;
            }

            // Mark transform and descendants as dirty if modified
            if (modified) {
                transform->isDirty = true;
            }
        }
    }

    // Clean up
    for (int j = 0; j < entity_count; j++) {
        RL_FREE(entity_names[j]);
    }
    RL_FREE(entity_names);
    RL_FREE(entity_ids);
    RL_FREE(name_list);
    ecs_query_fini(query);
}

//===============================================
// Node 2D Helper
//===============================================

ecs_entity_t create_node2d_text(ecs_world_t *world, float x, float y, char *text) {
    ecs_entity_t entity = ecs_new(world);
    ecs_set(world, entity, rect_t, {
        .rect = (Rectangle){0, 0, 120, 24} // Default rectangle values
    });
    ecs_set(world, entity, transform_2d_t, {
        .local_pos = (Vector2){x, y}, 
        .world_pos = (Vector2){0, 0},
        .local_scale = (Vector2){1, 1},
        .local_rotation = 0,
        .world_rotation = 0,
        .isDirty = true
    });
    char text_buffer[256] = {0}; // Temporary buffer
    strncpy(text_buffer, text ? text : "", sizeof(text_buffer) - 1); // Safe copy
    text_buffer[sizeof(text_buffer) - 1] = '\0'; // Ensure null-termination
    ecs_set(world, entity, text_box_t, {
        .text = {0}, // Initialize array
        .edit_mode = true
    });
    text_box_t *text_box = ecs_get_mut(world, entity, text_box_t);
    strncpy(text_box->text, text_buffer, sizeof(text_box->text) - 1);
    text_box->text[sizeof(text_box->text) - 1] = '\0';

    // ecs_set(world, entity, text_box_t, {
    //     .text = text,
    //     .edit_mode = true // Default edit mode
    // });
    return entity;
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
    ECS_COMPONENT_DEFINE(world, transform_2d_t);
    ECS_COMPONENT_DEFINE(world, transform_2d_select_t);
    

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


    // Transform system
    ecs_system(world, {
        .entity = ecs_entity(world, { 
            .name = "update_transform_2d_system", 
            .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
        }),
        .query.terms = {
            { .id = ecs_id(transform_2d_t) }
        },
        .callback = update_transform_2d_system
    });


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
    // ecs_system(world, {
    //   .entity = ecs_entity(world, { .name = "render2d_grid_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
    //   .callback = render2d_grid_system
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
    // render 2d, text drag
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_drag_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_t)}, // 
        { .id = ecs_id(transform_2d_t)} // 
      },
      .callback = render2d_text_drag_system
    });

    // render 2d, text box
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_box_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_box_t)}, // 
        { .id = ecs_id(transform_2d_t)} // 
      },
      .callback = render2d_text_box_system
    });
    // render 2d, text box, move
    ecs_system(world, {
      .entity = ecs_entity(world, { .name = "render2d_text_box_drag_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
      .query.terms = {
        { .id = ecs_id(rect_t)}, // 
        { .id = ecs_id(text_box_t)}, // 
        { .id = ecs_id(transform_2d_t)} // 
      },
      .callback = render2d_text_box_drag_system
    });

//===============================================
// CONNECTORS
//===============================================

    // ecs_system(world, {
    //     .entity = ecs_entity(world, { .name = "render2d_connector_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
    //     .query.terms = {
    //         { .id = ecs_id(connector_t) }
    //     },
    //     .callback = render2d_connector_system
    // });

    // ecs_system(world, {
    //     .entity = ecs_entity(world, { .name = "connector_creation_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
    //     .query.terms = {
    //         { .id = ecs_id(rect_t) }
    //     },
    //     .callback = connector_creation_system
    // });


    // ecs_system(world, {
    //     .entity = ecs_entity(world, { .name = "render_2d_draw_pins_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
    //     .query.terms = {
    //         { .id = ecs_id(rect_t) },
    //         { .id = ecs_id(connector_pin_t) }
    //     },
    //     .callback = render_2d_draw_pins_system
    // });

    // ecs_system(world, {
    //     .entity = ecs_entity(world, { .name = "connector_pin_creation_system", .add = ecs_ids(ecs_dependson(LogicUpdatePhase)) }),
    //     .query.terms = {
    //         { .id = ecs_id(rect_t) },
    //         { .id = ecs_id(connector_pin_t) },
    //     },
    //     .callback = connector_pin_creation_system
    // });

    // ecs_system(world, {
    //     .entity = ecs_entity(world, { .name = "render_2d_connector_status_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
    //     .query.terms = {
    //         { .id = ecs_id(mouse_t), .src.id = ecs_id(mouse_t)  }
    //     },
    //     .callback = render_2d_connector_status_system
    // });

//===============================================
// 
//===============================================

    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "render_2d_transform_list_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
        .query.terms = {
            { .id = ecs_id(transform_2d_select_t), .src.id = ecs_id(transform_2d_select_t) } // Singleton
            
        },
        .callback = render_2d_transform_list_system
    });


    ecs_system(world, {
        .entity = ecs_entity(world, { .name = "update_transform_2d_rect_system", .add = ecs_ids(ecs_dependson(RenderPhase)) }),
        .query.terms = {
            { .id = ecs_id(transform_2d_t) },
            { .id = ecs_id(rect_t) },
            
        },
        .callback = update_transform_2d_rect_system
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
    ecs_set(world, text1, transform_2d_t, {
        .local_pos = {0, 20}, 
        .world_pos = {0, 0},
        .local_scale = {1, 1},
        .local_rotation = 0,
        .world_rotation = 0,
        .isDirty = true
    });
    ecs_set(world, text1, rect_t, {
        .rect = (Rectangle){0,0,120,24}
    });
    ecs_set(world, text1, text_t, {
        .text = "flecs test1!"
    });
    
    ecs_entity_t text2 = ecs_new(world);
    ecs_set(world, text2, transform_2d_t, {
        .local_pos = {0, 40}, 
        .world_pos = {0, 0},
        .local_scale = {1, 1},
        .local_rotation = 0,
        .world_rotation = 0,
        .isDirty = true
    });
    ecs_set(world, text2, rect_t, {
        .rect = (Rectangle){0,0,120,24}
    });
    ecs_set(world, text2, text_t, {
        .text = "flecs test2!"
    });

    ecs_singleton_set(world,transform_2d_select_t,{
        .id = 0
    });
    



    // ecs_entity_t textbox1 = ecs_new(world);
    // ecs_set(world, textbox1, rect_t, {
    //     .rect = (Rectangle){0,50,120,24},
    // });
    // ecs_set(world, textbox1, text_box_t, {
    //     // .rect = (Rectangle){0,50,120,24},
    //     .text = "flecs test!",
    //     .edit_mode = true
    // });

    ecs_entity_t textbox1 = create_node2d_text(world, 10, 50,"Hello Flecs!");
    // ecs_set(world, textbox1, transform_2d_t, {
    //     .local_pos = {10, 50}, 
    //     .world_pos = {10, 50},
    //     .local_scale = {1, 1},
    //     .local_rotation = 0,
    //     .world_rotation = 0,
    //     .isDirty = true
    // });


    ecs_entity_t textbox2 = ecs_new(world);
    ecs_set(world, textbox2, rect_t, {
        .rect = (Rectangle){0,0,120,24},
    });
    ecs_set(world, textbox2, transform_2d_t, {
        .local_pos = {0, 80}, 
        .world_pos = {0, 0},
        .local_scale = {1, 1},
        .local_rotation = 0,
        .world_rotation = 0,
        .isDirty = true
    });
    ecs_set(world, textbox2, text_box_t, {
        .text = "flecs test!2",
        .edit_mode = true
    });
    


    // ecs_entity_t connector1 = ecs_new(world);
    // ecs_set(world, connector1, connector_t, {
    //     .in = text1,
    //     .out = textbox1
    // });
    // ecs_entity_t connector_pin1 = ecs_new(world);
    // ecs_set(world, connector_pin1, rect_t, {
    //     .rect = (Rectangle)
    //     {
    //         10,
    //         100,
    //         pin_size,
    //         pin_size
    //     }
    // });
    // ecs_set(world, connector_pin1, connector_pin_t, {
    //     .pin = PIN_PLACE_HOLDER,
    //     .dir = PIN_IN
    // });

    // ecs_entity_t connector_pin2 = ecs_new(world);
    // ecs_set(world, connector_pin2, rect_t, {
    //     .rect = (Rectangle)
    //     {
    //         10,
    //         200,
    //         pin_size,
    //         pin_size
    //     }
    // });
    // ecs_set(world, connector_pin2, connector_pin_t, {
    //     .pin = PIN_PLACE_HOLDER,
    //     .dir = PIN_IN
    // });

    // ecs_entity_t connector_pin3 = ecs_new(world);
    // ecs_set(world, connector_pin3, rect_t, {
    //     .rect = (Rectangle)
    //     {
    //         150,
    //         250,
    //         pin_size,
    //         pin_size
    //     }
    // });
    // ecs_set(world, connector_pin3, connector_pin_t, {
    //     .pin = PIN_PLACE_HOLDER,
    //     .dir = PIN_IN
    // });

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