// #define COW_PATCH_FRAMERATE
// #define COW_PATCH_FRAMERATE_SLEEP
#include "include.cpp"

char* FACE_LEFT = "duck_sprite/face_left.png";
char* FACE_RIGHT = "duck_sprite/face_right.png"; 
char* LEFT_WALK_1 = "duck_sprite/walk_left1.png";
char* LEFT_WALK_2 = "duck_sprite/walk_left2.png";
char* RIGHT_WALK_1 = "duck_sprite/walk_right1.png";
char* RIGHT_WALK_2 = "duck_sprite/walk_right2.png";
char* LEFT_ATTACK = "duck_sprite/jab_left.png";
char* RIGHT_ATTACK = "duck_sprite/jab_right.png";

vec2 DUCK_SZ = {3.0, 3.0};

char* walk_left[2] = {LEFT_WALK_1, LEFT_WALK_2};
char* walk_right[2] = {RIGHT_WALK_1, RIGHT_WALK_2};

int frame = 0;
bool game_over = false;

enum Direction {
    LEFT, RIGHT,
};

enum CollisionType {
    NONE,
    TOP_COLLISION,
    BOTTOM_COLLISION,
    LEFT_COLLISION,
    RIGHT_COLLISION,
};

struct Duck {
    vec2 position;
    int platform = NULL;
    real velocity = 0.5;
    vec2 duck_limits = {0, 0};
    vec2 delta = {0, 0};
    Direction direction = LEFT;
    char* texture = FACE_LEFT;
    bool alive = true;
    bool on_ground = true;
    int jump = 0;
    int attack = 0;
};

struct Land {
    vec2 position;
    vec2 size;
    vec3 color;
};

CollisionType check_collision(vec2 pos_1, vec2 size_1, vec2 pos_2, vec2 size_2) {
    // collision x-axis?
    CollisionType collision = NONE;
    bool collision_x = pos_1.x + size_1.x >= pos_2.x &&
        pos_2.x + size_2.x >= pos_1.x;
    // collision y-axis?
    bool collision_y = pos_1.y + size_2.y >= pos_2.y &&
       pos_2.y + size_2.y >= pos_1.y;

    if (collision_x && (round((pos_2.y + size_2.y) - pos_1.y) == 0)) {
        collision = BOTTOM_COLLISION;
    } else if (collision_y && (round(pos_2.x - (pos_1.x + size_1.x)) == 0)) {
        collision = LEFT_COLLISION;
    } else if (collision_y && (round((pos_2.x + size_2.x) - pos_1.x) == 0)) {
        collision = RIGHT_COLLISION;
    }

    return collision;
}

void update_duck(Duck* duck) {
    duck->delta = {0, 0};
    duck->texture = duck->direction == LEFT ? FACE_LEFT : FACE_RIGHT;
    int index = 0;

    // jumping
    if (duck->jump > 0) {
        duck->delta += {0, 2};
        duck->jump -= 1;
    }

    // initiate jump
    if (globals.key_pressed['w'] && !(duck->jump) && (duck->on_ground)) {
        duck->jump = 10;
        duck->position += {0, 2};
    }

    // move left and right
    if (globals.key_held['a']) {
        duck->direction = LEFT;
        //duck->texture = FACE_LEFT;
        duck->delta += {-duck->velocity, 0.0};
        index = index == 1 ? 0 : 1;
        duck->texture = walk_left[frame/6%2];
    
    } else if (globals.key_held['d']) {
        duck->direction = RIGHT;
        //duck->texture = FACE_RIGHT;
        duck->delta += {duck->velocity, 0.0};
        index = index == 1 ? 0 : 1;
        duck->texture = walk_right[frame/6%2];
    }

    //attacking
    if (duck->attack > 0) {
        duck->texture = duck->direction == LEFT ? LEFT_ATTACK : RIGHT_ATTACK;
        duck->attack -= 1;
    }
    
    // attack!
    if (globals.key_pressed[COW_KEY_SPACE]) {
        duck->attack = 5;
    } 
}

void generate_new_platform(Land prev, vec2 y_limits, StretchyBuffer<Land>* platforms, StretchyBuffer<Duck>* ducks) {
        real max_x = prev.position.x + prev.size.x + 10;
        real min_x = prev.position.x + prev.size.x + 5;

        real max_y = y_limits[1] - 10;
        real min_y = y_limits[0] + 5;

        real y = random_real(prev.position.y - 10, prev.position.y + 10);
        vec2 position = {random_real(min_x, max_x), CLAMP(y, min_y, max_y)};
        vec2 size = {random_real(10, 30), 3.0};

        if (size.x > 20) {
            vec2 duck_position = {
                position.x + size.x/2,
                position.y + size.y
            };

            vec2 platform_limits = {position.x, position.x + size.x - 3.0};

            sbuff_push_back(ducks, {duck_position, platforms->length, 0.1, platform_limits});
        }
        sbuff_push_back(platforms, {position, size, monokai.green});
}

void init_platforms(vec2 y_limits, StretchyBuffer<Land>* platforms, StretchyBuffer<Duck>* ducks) {
    srand(time(NULL));

    Land initial_platform = {{-25.0, -25.0}, {50.0, 3.0}, monokai.gray};
    sbuff_push_back(platforms, initial_platform);
    
    for (int i = 1; i < 10; i++) {
        Land prev = platforms->data[i-1];
        generate_new_platform(prev, y_limits, platforms, ducks);
    }
}

void init_background(StretchyBuffer<real>* backgrounds, real screen_width, real screen_height) {
    vec3 background_vp1[] = {
        {0.0 - screen_height, -screen_height/2, 0},
        {0.0 + screen_height, -screen_height/2, 0},
        {0.0 + screen_height, screen_height/2, 0},
        {0.0 - screen_height, screen_height/2, 0}
    };

    vec3 background_vp2[] = {
        {background_vp1[0][0] + screen_width, -screen_height/2, 0},
        {background_vp1[1][0] + screen_width, -screen_height/2, 0},
        {background_vp1[2][0] + screen_width, screen_height/2, 0},
        {background_vp1[3][0] + screen_width, screen_height/2, 0}
    };

    sbuff_push_back(backgrounds, background_vp1[0].x);
    sbuff_push_back(backgrounds, background_vp2[0].x);
}

void draw_platforms(mat4 PV, mat4 V, mat4 M, int3 triangle_indices[], StretchyBuffer<Land> platforms) {
    char* platform_texture = "elements/platform.png";
    for (int i = 0; i < platforms.length; i++) {
        vec2 texture_coords[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        Land platform = platforms[i];

        vec3 platform_vp[] = {
            {platform.position.x, platform.position.y, 0},
            {platform.position.x + platform.size.x, platform.position.y, 0},
            {platform.position.x + platform.size.x, platform.position.y + platform.size.y, 0},
            {platform.position.x, platform.position.y + platform.size.y, 0},

        };  
        mesh_draw(PV, V, M, 2, triangle_indices, 4, platform_vp, NULL, NULL, {}, texture_coords, platform_texture);
    }
}

void draw_ducks(mat4 PV, mat4 V, mat4 M, int3 triangle_indices[], StretchyBuffer<Duck> ducks) {
    vec2 texture_coords[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (int i = 0; i < ducks.length; i++) {
        Duck duck = ducks[i];

        vec3 duck_vp[] = {
            {duck.position.x, duck.position.y, 0},
            {duck.position.x + DUCK_SZ.x, duck.position.y, 0},
            {duck.position.x + DUCK_SZ.x, duck.position.y + DUCK_SZ.y, 0},
            {duck.position.x, duck.position.y + DUCK_SZ.y, 0},
        };

        mesh_draw(PV, V, M, 2, triangle_indices, 4, duck_vp, NULL, NULL, {}, texture_coords, duck.texture);
    }
}

void update_competition(Duck* duck) {
    vec2 platform_limits = duck->duck_limits;

    if (!duck->alive) {
        duck->position.y -= 1.0;

    } else {
        if (duck->direction == LEFT) {
            duck->position.x -= duck->velocity;
            duck->texture = walk_left[frame/6%2];
        } else {
            duck->position.x += duck->velocity;
            duck->texture = walk_right[frame/6%2];
        }

        if (duck->position.x <= platform_limits[0]) {
            duck->direction = RIGHT;
        } else if (duck->position.x >= platform_limits[1]) {
            duck->direction = LEFT;
        }
    }
}

void update_platforms(int i, StretchyBuffer<Land>* platforms, StretchyBuffer<Duck>* ducks, real x_limit, vec2 y_limits) {
    Land platform = platforms->data[i];

    if (platform.position.x + platform.size.x < x_limit) {
        Land last = platforms->data[platforms->length-1];
        generate_new_platform(last, y_limits, platforms, ducks);
        sbuff_delete(platforms, i);
    }
}

void handle_duck_collision(CollisionType collision_type, Duck* main_duck, int i, StretchyBuffer<Duck>* ducks) {
    Duck *other_duck = &ducks->data[i];
    if (collision_type == LEFT_COLLISION) {
        if (main_duck->attack > 0) {
            other_duck->alive = false;
        } else {
            main_duck->delta.x = other_duck->direction == LEFT ? main_duck->delta.x - 2 : 0;
        }

    } else if (collision_type == RIGHT_COLLISION) {
        if (main_duck->attack > 0) {
            other_duck->alive = false;
        } else {
            main_duck->delta.x = other_duck->direction == RIGHT ? main_duck->delta.x + 2 : 0;
        }
    }
}

void update_background(StretchyBuffer<real>* backgrounds, Camera2D camera) {
    if (backgrounds->data[0] + camera.screen_height_World*2 < camera.o_x - camera.screen_height_World) {
        sbuff_delete(backgrounds, 0);
        real new_background = backgrounds->data[1] + camera.screen_height_World*2;
        sbuff_push_back(backgrounds, new_background);
    }
}

void draw_backgrounds(mat4 PV, mat4 V, mat4 M, real screen_width, real screen_height, int3 triangle_indices[], StretchyBuffer<real>* backgrounds) {
    vec2 texture_coords[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    char* background_texture = "elements/clouds.png";

    vec3 bg_vp1[] = {
        {backgrounds->data[0], -screen_height/2, 0}, 
        {backgrounds->data[0] + screen_width, -screen_height/2, 0},
        {backgrounds->data[0] + screen_width, screen_height/2, 0},
        {backgrounds->data[0], screen_height/2, 0}
    };

    vec3 bg_vp2[] = {
        {backgrounds->data[1], -screen_height/2, 0},
        {backgrounds->data[1] + screen_width, -screen_height/2, 0},
        {backgrounds->data[1] + screen_width, screen_height/2, 0},
        {backgrounds->data[1], screen_height/2, 0}
    };

    mesh_draw(PV, V, M, 2, triangle_indices, 4, bg_vp1, NULL, NULL, {}, texture_coords, background_texture);
    mesh_draw(PV, V, M, 2, triangle_indices, 4, bg_vp2, NULL, NULL, {}, texture_coords, background_texture);
}

void reset_game(Camera2D* camera, Duck* main_duck, real* time, real* scroll_speed, StretchyBuffer<Land>* platforms, StretchyBuffer<Duck>* ducks, StretchyBuffer<real>* backgrounds) {
    camera->o_x = 0.0;
    camera->o_y = 0.0;

    real screen_width = camera->screen_height_World*2; 
    real screen_height = camera->screen_height_World;
    vec2 y_limits = {0.0 - screen_height/2, 0.0 + screen_height/2};

    sbuff_free(platforms);
    sbuff_free(ducks);
    sbuff_free(backgrounds);

    init_platforms(y_limits, platforms, ducks);
    init_background(backgrounds, screen_width, screen_height);
    
    main_duck->position = {0, 0};

    *time = 0.0;
    *scroll_speed = 0.2;
}


void final() {
    Camera2D camera = {50.0, 0.0, 0.0};
    mat4 V = globals.Identity;
    mat4 M = globals.Identity;
    real screen_width = camera.screen_height_World*2; 
    real screen_height = camera.screen_height_World;
    
    struct Duck main_duck = {{0, 0}};

    int3 triangle_indices[] = {{0, 1, 2}, {0, 2, 3}};
    vec2 texture_coords[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    real time = 0.0;
    real high_score = time;
    real scroll_speed = 0.2;
    vec2 gravity = {0, -0.9};
    bool collision = false;

    vec2 y_limits = {0.0 - screen_height/2, 0.0 + screen_height/2};
    
    StretchyBuffer<Land> platforms = {};
    StretchyBuffer<Duck> ducks = {};
    StretchyBuffer<real> backgrounds = {};

    init_platforms(y_limits, &platforms, &ducks);
    init_background(&backgrounds, screen_width, screen_height);

    while (cow_begin_frame()) {
        mat4 PV = camera_get_PV(&camera);
        real x_limit = camera.o_x - 2*screen_height;
        gui_readout("high score", &high_score);
        gui_readout("time", &time);

        if (main_duck.position.x < camera.o_x - screen_height 
        || main_duck.position.y < camera.o_y - screen_height/2) {
            game_over = true;
        }

        if (game_over) {
            gui_printf("game over :( press space to play again!");
            if (time > high_score) { high_score = time; }

            if (gui_button("play again", COW_KEY_SPACE)) {
                reset_game(&camera, &main_duck, &time, &scroll_speed, &platforms, &ducks, &backgrounds);
                game_over = false;
            }
        }
        
        update_duck(&main_duck);
        update_background(&backgrounds, camera);
        draw_backgrounds(PV, V, M, screen_width, screen_height, triangle_indices, &backgrounds);

        for (int i = 0; i < ducks.length; i++) {
            Land current_platform = platforms[ducks[i].platform];
            if (ducks[i].position.y < y_limits[0]) {
                sbuff_delete(&ducks, i);
            }
            update_competition(&ducks[i]);

            CollisionType collision_type = check_collision(main_duck.position, DUCK_SZ, ducks[i].position, DUCK_SZ);
            handle_duck_collision(collision_type, &main_duck, i, &ducks);
        }

        for (int i = 0; i < platforms.length; i++) {
            Land platform = platforms[i];

            update_platforms(i, &platforms, &ducks, x_limit, y_limits);

            CollisionType collision_type = check_collision(main_duck.position, DUCK_SZ, platform.position, platform.size);
            if (collision_type == BOTTOM_COLLISION) {
                main_duck.jump = 0;
                main_duck.position.y = platform.position.y + platform.size.y;
                main_duck.on_ground = true;
                collision = true;
            }
        }

        if (!collision) {
            main_duck.delta += gravity;
            main_duck.on_ground = false;
        }
        
        main_duck.position += main_duck.delta;
        
        vec3 duck_vp[] = {
            {main_duck.position.x, main_duck.position.y, 0},
            {main_duck.position.x + DUCK_SZ.x, main_duck.position.y, 0},
            {main_duck.position.x + DUCK_SZ.x, main_duck.position.y + DUCK_SZ.y, 0},
            {main_duck.position.x, main_duck.position.y + DUCK_SZ.y, 0},
        };

        mesh_draw(PV, V, M, 2, triangle_indices, 4, duck_vp, NULL, NULL, {}, texture_coords, main_duck.texture);
        
        draw_platforms(PV, V, M, triangle_indices, platforms);
        draw_ducks(PV, V, M, triangle_indices, ducks);

        frame += 1;
        collision = false;
        camera.o_x += scroll_speed + 0.001*(time);

        if (!game_over) { time += 0.0167; }
    }

    sbuff_free(&backgrounds);
    sbuff_free(&platforms);
    sbuff_free(&ducks);
}


int main() {
    APPS {
        APP(final);
    }
    return 0;
}





