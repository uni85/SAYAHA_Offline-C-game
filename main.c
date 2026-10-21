#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// ============================================================================
// CONSTANTS & TYPES
// ============================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define SCALE_FACTOR 6
#define TARGET_FPS 60
#define WALL_MARGIN 6.0f
#define LEFT_BOUNDARY 64.0f

#define WIRE_Y 50.0f
#define WIRE_POLE1_X 579.0f
#define WIRE_POLE2_X 659.0f
#define SHOE_X 619.0f

#define ZONE5_LIGHT_X 1169.0f

#define TOTAL_SCENARIOS 16

#define P8_BLACK       CLITERAL(Color){ 0, 0, 0, 255 }
#define P8_DARK_BLUE   CLITERAL(Color){ 29, 43, 83, 255 }
#define P8_BUILDING_BLUE CLITERAL(Color){ 50, 68, 120, 255 }
#define P8_DARK_PURP   CLITERAL(Color){ 126, 37, 83, 255 }
#define P8_DARK_GREEN  CLITERAL(Color){ 0, 135, 81, 255 }
#define P8_GROUND_GREEN CLITERAL(Color){ 6, 76, 45, 255 }
#define P8_BROWN       CLITERAL(Color){ 171, 82, 54, 255 }
#define P8_DARK_GRAY   CLITERAL(Color){ 95, 87, 79, 255 }
#define P8_POLE_DARK   CLITERAL(Color){ 35, 33, 31, 255 }
#define P8_WHITE       CLITERAL(Color){ 255, 241, 232, 255 }
#define P8_YELLOW      CLITERAL(Color){ 255, 236, 39, 255 }
#define P8_GREEN       CLITERAL(Color){ 0, 228, 54, 255 }
#define P8_RED         CLITERAL(Color){ 255, 0, 77, 255 }
#define P8_PEACH       CLITERAL(Color){ 255, 204, 170, 255 }
#define P8_TITLE_BG    CLITERAL(Color){ 8, 45, 20, 255 }
#define P8_WATER       CLITERAL(Color){ 40, 90, 170, 210 }

// Audio Synthesis Constants
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_STREAM_BUFFER_SIZE 1024

// Note Frequencies
#define NOTE_C2  65.41f
#define NOTE_D2  73.42f
#define NOTE_EB2 77.78f
#define NOTE_E2  82.41f
#define NOTE_F2  87.31f
#define NOTE_G2  98.00f
#define NOTE_AB2 103.83f
#define NOTE_A2  110.00f
#define NOTE_BB2 116.54f
#define NOTE_B2  123.47f
#define NOTE_C3  130.81f
#define NOTE_D3  146.83f
#define NOTE_EB3 155.56f
#define NOTE_E3  164.81f
#define NOTE_F3  174.61f
#define NOTE_G3  196.00f
#define NOTE_AB3 207.65f
#define NOTE_A3  220.00f
#define NOTE_BB3 233.08f
#define NOTE_B3  246.94f
#define NOTE_C4  261.63f
#define NOTE_D4  293.66f
#define NOTE_E4  329.63f

// Extended 64-step Melodic Lead
static const float sawyer_riff[] = {
    // Phrase 1 (Main Loop)
    NOTE_E3,  0.0f,    NOTE_G3, NOTE_A3,  0.0f,    NOTE_G3, NOTE_E3,  0.0f,
    NOTE_D3,  NOTE_E3, NOTE_G3, 0.0f,     NOTE_A3,  NOTE_B3, NOTE_A3,  NOTE_G3,
    NOTE_E3,  0.0f,    NOTE_G3, NOTE_A3,  0.0f,    NOTE_C4, NOTE_B3,  NOTE_G3,
    NOTE_A3,  NOTE_G3, NOTE_E3, NOTE_D3,  NOTE_E3,  0.0f,    0.0f,     0.0f,
    // Phrase 2 (Variation / Switch-up)
    NOTE_G3,  0.0f,    NOTE_A3, NOTE_C4,  0.0f,    NOTE_B3, NOTE_A3,  NOTE_G3,
    NOTE_E3,  NOTE_G3, NOTE_A3, 0.0f,     NOTE_G3,  NOTE_E3, NOTE_D3,  NOTE_B2,
    NOTE_C3,  0.0f,    NOTE_E3, NOTE_G3,  0.0f,    NOTE_A3, NOTE_B3,  NOTE_C4,
    NOTE_D4,  NOTE_C4, NOTE_B3, NOTE_A3,  NOTE_G3,  NOTE_E3, 0.0f,     0.0f
};
static const int sawyer_length = 64;

// 32-step Boom-Bap / Street Hip-Hop Bassline
static const float street_bass[] = {
    NOTE_E2, 0.0f,    0.0f,    NOTE_E2, 0.0f,    0.0f,    NOTE_G2, 0.0f,
    NOTE_C2, 0.0f,    NOTE_C2, 0.0f,    0.0f,    0.0f,    NOTE_D2, 0.0f,
    NOTE_E2, 0.0f,    0.0f,    NOTE_E2, 0.0f,    NOTE_E2, NOTE_G2, 0.0f,
    NOTE_A2, 0.0f,    0.0f,    NOTE_G2, NOTE_F2, 0.0f,    NOTE_E2, 0.0f
};
static const int street_length = 32;

static float audio_time = 0.0f;

typedef enum { MODE_PLAY, MODE_WALK_TO_WALL, MODE_SHADOW_TAKEOVER } GameMode;
typedef enum { STICK_WORLD, STICK_PLAYER, STICK_SHADOW } StickOwner;
typedef enum { NPC_WALK, NPC_STEAL_STICK, NPC_LOOK, NPC_TALKING, NPC_WAVING } NPCState;
typedef enum { SHADOW_HIDDEN, SHADOW_LURKING } ShadowState;
typedef enum { SCREEN_TITLE, SCREEN_GAME, SCREEN_END } AppScreen;

typedef struct { const char* speaker; const char* text; } DialogueLine;
typedef struct { int id; float start_x; float end_x; } ShadowZone;

typedef struct {
    float x, y, ground_y, dy;
    bool is_jumping;
    float speed;
    bool flip, is_walking;
    NPCState npc_state;
    int state_timer;
    float target_x;
    int dialogue_step;
} Player;

typedef struct {
    float x, y;
    bool active, is_thrown;
    float h_speed;
    bool throw_flip;
    StickOwner owner;
} Stick;

typedef struct {
    float x, y;
    ShadowZone zone;
    bool active, flip;
    ShadowState state;
    bool waving;
} ShadowNPC;

typedef struct { float x, w, h; Color color; } Building;
typedef struct { float x, y, speed; bool flip, active; int type; } Car;

// ============================================================================
// AUDIO SYNTHESIS HELPERS & CALLBACK
// ============================================================================
// ============================================================================
// AUDIO SYNTHESIS HELPERS & CALLBACK
// ============================================================================
static float TriangleWave(float phase) {
    return 2.0f * fabsf(2.0f * (phase - floorf(phase + 0.5f))) - 1.0f;
}

static float SineWave(float phase) {
    return sinf(2.0f * PI * phase);
}

static float SquareWave(float phase) {
    return (sinf(2.0f * PI * phase) >= 0.0f) ? 0.2f : -0.2f;
}

void AudioMashupCallback(void *buffer, unsigned int frames) {
    float *output = (float *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        int lead_step = ((int)(audio_time * 4.5f)) % sawyer_length;
        int bass_step = ((int)(audio_time * 4.5f)) % street_length;

        float freq_lead = sawyer_riff[lead_step];
        float freq_bass = street_bass[bass_step];

        // Increased Lead Synth Volume
        float lead_synth = (freq_lead > 0.0f) ? TriangleWave(audio_time * freq_lead) * 0.30f : 0.0f;
        
        // Increased Hip-Hop Sub Bass & Grit Volume
        float bass_env   = 1.0f - fmodf(audio_time * 4.5f, 1.0f);
        float sub_bass   = (freq_bass > 0.0f) ? SineWave(audio_time * (freq_bass * 0.5f)) * 0.35f * powf(bass_env, 0.5f) : 0.0f;
        float bass_grit  = (freq_bass > 0.0f) ? SquareWave(audio_time * freq_bass) * 0.10f * powf(bass_env, 2.0f) : 0.0f;

        // Hip-Hop Drum Rhythm (Kept Low)
        float step_clock = audio_time * 9.0f;
        int drum_step = ((int)step_clock) % 16;
        float drum_env = 1.0f - fmodf(step_clock, 1.0f);

        float kick = 0.0f;
        if (drum_step == 0 || drum_step == 8 || drum_step == 10) {
            float kick_freq = 110.0f * powf(drum_env, 3.0f);
            kick = SineWave(audio_time * kick_freq) * powf(drum_env, 2.0f) * 0.04f; 
        }

        float snare = 0.0f;
        if (drum_step == 4 || drum_step == 12) {
            float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
            snare = (noise * 0.03f + SineWave(audio_time * 180.0f) * 0.015f) * powf(drum_env, 4.0f); 
        }

        float hihat_env = 1.0f - fmodf(audio_time * 9.0f, 1.0f);
        float hihat = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * powf(hihat_env, 12.0f) * 0.007f; 
        
        // Increased master output scalar for the overall mix
        float raw_mix = lead_synth + sub_bass + bass_grit + kick + snare + hihat;
        output[i] = raw_mix * 0.65f; 

        audio_time += 1.0f / (float)AUDIO_SAMPLE_RATE;
    }
}

// ============================================================================
// SPRITES
// ============================================================================
static const int SPRITE_PLAYER_IDLE[8][8] = {
    {0,0,1,1,1,1,0,0},{0,1,6,6,6,6,1,0},{0,1,1,6,1,6,1,0},{0,1,6,6,6,6,1,0},
    {0,0,8,8,8,8,0,0},{0,1,8,8,8,8,1,0},{0,0,1,0,0,1,0,0},{0,1,1,0,0,1,1,0}
};
static const int SPRITE_PLAYER_WALK_B[8][8] = {
    {0,0,1,1,1,1,0,0},{0,1,6,6,6,6,1,0},{0,1,1,6,1,6,1,0},{0,1,6,6,6,6,1,0},
    {0,0,8,8,8,8,0,0},{0,1,8,8,8,8,1,0},{0,0,1,0,0,1,1,0},{0,1,1,0,1,1,0,0}
};

static const int SPRITE_PLAYER_SHADOW_IDLE[8][8] = {
    {0,0,1,1,1,1,0,0},{0,1,4,4,4,4,1,0},{0,1,1,7,1,7,1,0},{0,1,4,4,4,4,1,0},
    {0,0,3,3,3,3,0,0},{0,1,3,3,3,3,1,0},{0,0,1,0,0,1,0,0},{0,1,1,0,0,1,1,0}
};
static const int SPRITE_PLAYER_SHADOW_WALK_B[8][8] = {
    {0,0,1,1,1,1,0,0},{0,1,4,4,4,4,1,0},{0,1,1,7,1,7,1,0},{0,1,4,4,4,4,1,0},
    {0,0,3,3,3,3,0,0},{0,1,3,3,3,3,1,0},{0,0,1,0,0,1,1,0},{0,1,1,0,1,1,0,0}
};
static const int SPRITE_PLAYER_SHADOW_WAVE[8][8] = {
    {0,7,1,1,1,1,0,0},{7,1,4,4,4,4,1,0},{0,1,1,7,1,7,1,0},{0,1,4,4,4,4,1,0},
    {0,0,3,3,3,3,0,0},{0,1,3,3,3,3,1,0},{0,0,1,0,0,1,0,0},{0,1,1,0,0,1,1,0}
};

static const int SPRITE_NPC_SHADOW_IDLE[8][8] = {
    {0,0,1,1,1,1,0,0},
    {0,1,4,4,4,4,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,4,4,4,4,1,0},
    {0,0,4,4,4,4,0,0},
    {0,1,4,4,4,4,1,0},
    {0,0,1,0,0,1,0,0},
    {0,1,1,0,0,1,1,0}
};
static const int SPRITE_NPC_SHADOW_WAVE[8][8] = {
    {0,1,1,1,1,1,0,0},
    {1,1,4,4,4,4,1,0},
    {0,1,1,1,1,1,1,0},
    {0,1,4,4,4,4,1,0},
    {0,0,4,4,4,4,0,0},
    {0,1,4,4,4,4,1,0},
    {0,0,1,0,0,1,0,0},
    {0,1,1,0,0,1,1,0}
};

static const Color P8_SPRITE_PALETTE[10] = {
    {0,0,0,0},{0,0,0,255},{29,43,83,255},{126,37,83,255},{95,87,79,255},
    {255,241,232,255},{255,204,170,255},{255,236,39,255},{255,0,77,255},{171,82,54,255}
};

// ============================================================================
// DIALOGUE SCRIPTS
// ============================================================================
static const DialogueLine ZONE_INTRO[] = {
    {"npc", "Oh, a new walker. You'll get used to the rhythm here soon enough."},
    {"player_shadow", "The rhythm?"},
    {"npc", "Step into the dark. We, the Shadows, live invisible, and we all need somewhere to just be ourselves, even if it's only a patch of shade."}
};
#define ZONE_INTRO_COUNT 3

static const DialogueLine DIALOGUE_ZONE1[] = {
    {"npc", "Hey, new in town?"},
    {"player_shadow", "Hmm, yes aren't we all?"},
    {"npc", "Kinda yes we come and go with walkers, but I have heard of better places where we even get to be permanent,"},
    {"npc", "Anyway do you have a cigarette?"}
};
#define DIALOGUE_ZONE1_COUNT 4

static const DialogueLine ZONE2_RETURN[] = {
    {"npc", "Oh hey I see you're back."},
    {"player_shadow", "Yes brought you a cigarette!"},
    {"npc", "Thanks, nice pun you made back there, I mean front there, ah whatever!"},
    {"player_shadow", "Which one? I do that a lot!"},
    {"npc", "Want a light? No that's not for me! Hahaha"},
    {"player_shadow", "Glad you're enlightened!"},
    {"npc", "Hahaha, then I have to announce to the winner, welcome to the gang."},
    {"npc", "And hey remember, we are all but one!"}
};
#define ZONE2_RETURN_COUNT 8

static const DialogueLine SCEN2_MAIN[] = {
    {"player_shadow", "Hey"},
    {"player_shadow", "HEY!"},
    {"npc", "Yes, are you talking to me? Sorry, distracted."},
    {"player_shadow", "By what?"},
    {"npc", "You see that shoe up there on the wires?"},
    {"player_shadow", "Yes?"},
    {"npc", "Well I need it, and don't ask why or how it ended up there, just big city problems!"}
};
#define SCEN2_MAIN_COUNT 7

static const DialogueLine SCEN2_WITH_STICK[] = {
    {"player_shadow", "I can throw this stick, and with the right angle we might become friends!"},
    {"npc", "Well that was... ok, let's see what you've got!"},
    {"player_shadow", "Ok."},
    {"action", "the stick sails up toward the shoe"},
    {"npc", "Oh wow, you really are good with angles. Thanks, I owe you one!"},
    {"npc", "A shoe is all that matters!"}
};
#define SCEN2_WITH_STICK_COUNT 6

static const DialogueLine SCEN2_WITHOUT_STICK[] = {
    {"player_shadow", "That's really high."},
    {"npc", "Yeah."}
};
#define SCEN2_WITHOUT_STICK_COUNT 2

static const DialogueLine ZONE5_CONVO[] = {
    {"npc", "Ha, you made it! Anything I can do for you?"},
    {"player_shadow", "Yes, I was wondering, why do you stay in this city? Can't you leave?"},
    {"npc", "Heh, I hear this a lot. Wanderers! This place has made us what we are, we're deeply attached to it."},
    {"player_shadow", "Yeah, but how would it work? Can you just walk out and never return? Has anyone ever tried?"},
    {"npc", "Yes, I've met wanderers who are from here. Believe it or not they're doing pretty well -- it's like they don't care about light and shadow anymore."},
    {"player_shadow", "How is that possible? What else is there?"},
    {"npc", "Well, I guess we all have to find our own way..."}
};
#define ZONE5_CONVO_COUNT 7

static const DialogueLine ZONE6_CONVO[] = {
    {"npc", "What up?"},
    {"player_shadow", "Hey, nothing much, what are you doing?"},
    {"npc", "Nothing much."},
    {"player_shadow", "Can I have a cigarette?"},
    {"npc", "Sure, there you go! Want a light? Seeing is believing, you know!"},
    {"player_shadow", "No no thanks, it's not for me!"}
};
#define ZONE6_CONVO_COUNT 6

static const DialogueLine ZONE8_CONVO[] = {
    {"npc", "Don't look at me, I am just an instrument!"}
};
#define ZONE8_CONVO_COUNT 1

static const DialogueLine ZONE7_WATER[] = {
    {"npc", "Hey I am so EXCITED! To ... to talk to someone!"},
    {"player_shadow", "Yes you must be lonely I understand."},
    {"npc", "Lonely? Maybe. Or maybe I just forgot there was a whole city on the other side of this puddle."},
    {"player_shadow", "The other side?"},
    {"npc", "Down here everything is upside down, but it still looks like a proper world, doesn't it? Sky beneath your feet, streets you can never quite walk."},
    {"player_shadow", "Is it a copy, or is it its own place?"},
    {"npc", "I used to think I was a copy. Now I think a reflection is just a place that only exists when someone is looking at it."},
    {"player_shadow", "Then what happens to you when no one's looking?"},
    {"npc", "The same thing that happens to a thought no one finishes. I don't disappear. I just... wait."}
};
#define ZONE7_WATER_COUNT 9

// ============================================================================
// GLOBAL STATE
// ============================================================================
static AppScreen app_screen = SCREEN_TITLE;
static float title_scroll = 0.0f;
static int title_car_timer = 0;

static GameMode mode = MODE_PLAY;
static Player player;
static Stick stick;

static ShadowZone szones[7] = {
    {1, 184.0f, 244.0f},
    {2, 364.0f, 424.0f},
    {3, 554.0f, 674.0f},
    {4, 854.0f, 974.0f},
    {5, 1154.0f, 1274.0f},
    {6, 1454.0f, 1574.0f},
    {8, 1874.0f, 1994.0f}
};
#define NUM_ZONES 7

static ShadowZone water_zone = {7, 1754.0f, 1854.0f};

static ShadowNPC shadow_npcs[8];
static Building skyline_far[30];
static Building skyline_blue[15];
static Building skyline_mid[20];

#define MAX_CARS 30
static Car cars[MAX_CARS];
static int car_timer = 0;
static float far_layer_width = 30 * 16;
static float blue_layer_width = 15 * 32;
static float mid_layer_width = 20 * 24;

static bool in_shadow = false;
static ShadowZone *current_zone = NULL;
static float cam_x = 0, cam_y = 0;
static float wall_target_x = 0.0f;
static bool last_entry_from_start = false;

static float shadow_x = 0;
static bool shadow_flip = false;
static bool shadow_detached = false;
static bool shadow_returning = false;
static bool shadow_chasing = false;

static float camera_zoom = 1.0f, camera_zoom_target = 1.0f;
static float focus_x = 64.0f, focus_y = 64.0f;

static bool zone2_branch_chosen = false;
static bool zone2_use_stick_branch = false;
static int action_timer = 0;
static float stick_anim_x = 0.0f, stick_anim_y = 0.0f;
static bool shoe_retrieved = false;

static int zone4_total_visits = 0;
static bool zone4_first_side_is_start = false;
static bool zone4_last_side_is_start = false;
static int zone4_side_switches = 0;
static const char* zone4_lines[2];
static int zone4_line_count = 1;

static bool zone5_zapped = false;
static bool completed_zone6 = false;

static float ripple_timer = 0.0f;

static int player_walk_counter = 0;
static int shadow_walk_counter = 0;

static int scenarios_completed = 0;
static float window_lit_chance = 0.4f;

static unsigned int rng_state = 42;
static void pico_srand(unsigned int seed) { rng_state = seed; }
static float pico_rnd(float max) {
    rng_state = rng_state * 1103515245 + 12345;
    return fmodf((float)rng_state / 65536.0f, max);
}

int ComputeLineTimer(const char* text) {
    int len = (int)strlen(text);
    int frames = (int)((90 + len * 4) * 1.15f);
    if (frames > 1035) frames = 1035;
    if (frames < 104) frames = 104;
    return frames;
}

int Zone2TotalLines(void) {
    return SCEN2_MAIN_COUNT + (zone2_use_stick_branch ? SCEN2_WITH_STICK_COUNT : SCEN2_WITHOUT_STICK_COUNT);
}
const DialogueLine* GetZone2Line(int step) {
    if (step < SCEN2_MAIN_COUNT) return &SCEN2_MAIN[step];
    int b = step - SCEN2_MAIN_COUNT;
    if (zone2_use_stick_branch) { if (b < SCEN2_WITH_STICK_COUNT) return &SCEN2_WITH_STICK[b]; }
    else { if (b < SCEN2_WITHOUT_STICK_COUNT) return &SCEN2_WITHOUT_STICK[b]; }
    return NULL;
}

const DialogueLine* GetZone2CigLine(int step, int* out_count) {
    if (completed_zone6) {
        *out_count = ZONE2_RETURN_COUNT;
        if (step < ZONE2_RETURN_COUNT) return &ZONE2_RETURN[step];
    } else {
        *out_count = DIALOGUE_ZONE1_COUNT;
        if (step < DIALOGUE_ZONE1_COUNT) return &DIALOGUE_ZONE1[step];
    }
    return NULL;
}

void SpawnVehicle(float x, float y, float speed, bool flip, int type) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) { cars[i] = (Car){ x, y, speed, flip, true, type }; return; }
    }
}

// ============================================================================
// LOGIC
// ============================================================================
void CheckShadowZones(void) {
    if (mode != MODE_PLAY) return;
    in_shadow = false;
    current_zone = NULL;
    for (int i = 0; i < NUM_ZONES; i++) {
        if (player.x >= szones[i].start_x && player.x <= szones[i].end_x) {
            in_shadow = true;
            current_zone = &szones[i];
        }
    }
}

void UpdateShadowNPCs(void) {
    for (int i = 0; i < 7; i++) {
        bool player_in_zone = (player.x >= shadow_npcs[i].zone.start_x && player.x <= shadow_npcs[i].zone.end_x);
        bool scenario_active_here = (mode == MODE_WALK_TO_WALL || mode == MODE_SHADOW_TAKEOVER)
            && current_zone != NULL && shadow_npcs[i].zone.id == current_zone->id;
        bool visible = player_in_zone || scenario_active_here;

        if (visible) {
            if (!shadow_npcs[i].waving) {
                bool talking_to_this_npc = shadow_detached && scenario_active_here;
                shadow_npcs[i].flip = talking_to_this_npc ? (shadow_x < shadow_npcs[i].x) : (player.x < shadow_npcs[i].x);
            }
            shadow_npcs[i].state = SHADOW_LURKING;
        } else {
            shadow_npcs[i].state = SHADOW_HIDDEN;
        }
    }
}

void ResetShadowTakeover(void) {
    mode = MODE_PLAY;
    shadow_detached = false;
    shadow_returning = false;
    shadow_chasing = false;
    player.dialogue_step = 0;
    zone2_branch_chosen = false;
    zone2_use_stick_branch = false;
    action_timer = 0;
    zone5_zapped = false;
    shadow_npcs[7].state = SHADOW_HIDDEN;
    if (stick.owner == STICK_SHADOW) stick.owner = STICK_PLAYER;
    for (int i = 0; i < 7; i++) shadow_npcs[i].waving = false;
}

void BeginShadowTakeover(void) {
    mode = MODE_SHADOW_TAKEOVER;
    shadow_detached = true;
    player.is_walking = false;
    shadow_x = player.flip ? (player.x + 4.0f) : (player.x - 4.0f);
    shadow_flip = player.flip;
    player.dialogue_step = 0;
    zone2_branch_chosen = false;
    action_timer = 0;
    zone5_zapped = false;

    int zid = (current_zone != NULL) ? current_zone->id : 0;

    if (zid == 1) {
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[0].x + 16.0f;
    } else if (zid == 2) {
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[1].x + 16.0f;
    } else if (zid == 3) {
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[2].x + 16.0f;
    } else if (zid == 4) {
        zone4_total_visits++;
        bool this_side = last_entry_from_start;
        if (zone4_total_visits == 1) {
            zone4_lines[0] = "I had a dream last night and now I know the secret of the universe, I will tell it to the 3rd person that I see today!";
            zone4_line_count = 1;
            zone4_first_side_is_start = this_side;
            zone4_last_side_is_start = this_side;
        } else {
            if (this_side != zone4_last_side_is_start) zone4_side_switches++;
            zone4_last_side_is_start = this_side;
            if (zone4_side_switches >= 2) {
                zone4_lines[0] = "Well, well, well, here it goes. One's sorrow at the other side of the world can make the entire world suffer;";
                zone4_lines[1] = "one's happiness can make the entire world smile.";
                zone4_line_count = 2;
            } else if (this_side == zone4_first_side_is_start) {
                zone4_lines[0] = "Greetings to the first person!"; zone4_line_count = 1;
            } else {
                zone4_lines[0] = "Hey, second person!"; zone4_line_count = 1;
            }
        }
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[3].x + 16.0f;
    } else if (zid == 5) {
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[4].x + 16.0f;
    } else if (zid == 6) {
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[5].x + 16.0f;
    } else if (zid == 8) {
        player.npc_state = NPC_WALK;
        player.target_x = shadow_npcs[6].x + 16.0f;
    } else if (stick.owner == STICK_PLAYER) {
        player.npc_state = NPC_STEAL_STICK;
    } else {
        player.npc_state = NPC_WALK;
        player.state_timer = 30 + (int)pico_rnd(60);
        player.target_x = (current_zone != NULL) ? (player.flip ? current_zone->start_x : current_zone->end_x) : player.x;
    }
}

void UpdatePlayerControl(bool s_pressed) {
    player.is_walking = false;
    float zone8_width = szones[NUM_ZONES - 1].end_x - szones[NUM_ZONES - 1].start_x;
    float right_boundary = szones[NUM_ZONES - 1].end_x + zone8_width;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        if (player.x - player.speed >= LEFT_BOUNDARY) player.x -= player.speed;
        else player.x = LEFT_BOUNDARY;
        player.flip = true; player.is_walking = true;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        if (player.x + player.speed <= right_boundary) player.x += player.speed;
        else player.x = right_boundary;
        player.flip = false; player.is_walking = true;
    }

    if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_Z)) && !player.is_jumping && !in_shadow) {
        player.dy = -4.5f; player.is_jumping = true;
    }

    if (player.is_jumping) {
        player.y += player.dy;
        player.dy += 0.4f;
        if (player.y >= player.ground_y) { player.y = player.ground_y; player.dy = 0.0f; player.is_jumping = false; }
    }

    if (stick.owner == STICK_PLAYER) {
        if (IsKeyPressed(KEY_W)) {
            stick.owner = STICK_WORLD; stick.active = true; stick.is_thrown = true;
            stick.y = player.y - 6; stick.throw_flip = player.flip;
            stick.h_speed = player.flip ? -5.0f : 5.0f;
            stick.x = player.flip ? player.x - 8 : player.x + 8;
        }
    } else {
        if (stick.active && !stick.is_thrown && !player.is_jumping && fabsf(player.x - stick.x) < 10) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { stick.active = false; stick.owner = STICK_PLAYER; }
        }
    }

    bool player_on_water = (player.x >= water_zone.start_x && player.x <= water_zone.end_x);
    if (shadow_chasing && player_on_water && s_pressed) {
        shadow_chasing = false;
        mode = MODE_SHADOW_TAKEOVER;
        current_zone = &water_zone;
        float mid = (water_zone.start_x + water_zone.end_x) / 2.0f;
        shadow_x = mid - 12.0f;
        shadow_flip = false;
        shadow_npcs[7].x = mid + 14.0f;
        shadow_npcs[7].y = 100.0f;
        shadow_npcs[7].zone = water_zone;
        shadow_npcs[7].state = SHADOW_LURKING;
        shadow_npcs[7].flip = true;
        shadow_detached = true;
        player.npc_state = NPC_TALKING;
        player.dialogue_step = 0;
        player.state_timer = ComputeLineTimer(ZONE7_WATER[0].text);
        return;
    }

    if (in_shadow && s_pressed && current_zone != NULL) {
        float dist_to_start = fabsf(player.x - current_zone->start_x);
        float dist_to_end = fabsf(player.x - current_zone->end_x);
        bool from_start_side = (dist_to_start < dist_to_end);
        last_entry_from_start = from_start_side;
        wall_target_x = from_start_side ? (current_zone->start_x - WALL_MARGIN) : (current_zone->end_x + WALL_MARGIN);
        mode = MODE_WALK_TO_WALL;
    }
}

void UpdateWalkToWall(void) {
    player.is_walking = true;
    float diff = wall_target_x - player.x;
    float dist = fabsf(diff);
    if (dist > 0.01f) {
        float step = player.speed;
        if (step > dist) step = dist;
        player.x += (diff > 0.0f) ? step : -step;
        player.flip = (diff < 0.0f);
    }
    if (fabsf(wall_target_x - player.x) <= 0.01f) {
        player.x = wall_target_x;
        player.is_walking = false;
        BeginShadowTakeover();
    }
}

void UpdateShadowNPC(bool s_pressed) {
    if (s_pressed) {
        shadow_chasing = true;
        mode = MODE_PLAY;
        return;
    }

    if (current_zone != NULL && current_zone->id == 1) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[0].x + 16.0f;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }
            else { player.npc_state = NPC_TALKING; player.dialogue_step = 0; player.state_timer = ComputeLineTimer(ZONE_INTRO[0].text); }
        } else if (player.npc_state == NPC_TALKING) {
            player.state_timer--; shadow_flip = true;
            if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                player.dialogue_step++;
                if (player.dialogue_step < ZONE_INTRO_COUNT) {
                    player.state_timer = ComputeLineTimer(ZONE_INTRO[player.dialogue_step].text);
                } else {
                    player.npc_state = NPC_WAVING; shadow_npcs[0].waving = true;
                    shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
                }
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 2) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[1].x + 16.0f;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }
            else {
                player.npc_state = NPC_TALKING; player.dialogue_step = 0;
                int cnt; const DialogueLine* first = GetZone2CigLine(0, &cnt);
                player.state_timer = (first != NULL) ? ComputeLineTimer(first->text) : 300;
            }
        } else if (player.npc_state == NPC_TALKING) {
            player.state_timer--; shadow_flip = true;
            if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                player.dialogue_step++;
                int cnt; const DialogueLine* next = GetZone2CigLine(player.dialogue_step, &cnt);
                if (next != NULL) {
                    player.state_timer = ComputeLineTimer(next->text);
                } else if (completed_zone6) {
                    app_screen = SCREEN_END;
                    mode = MODE_PLAY;
                    shadow_detached = false;
                } else {
                    player.npc_state = NPC_WAVING; shadow_npcs[1].waving = true;
                    shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
                }
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 3) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[2].x + 16.0f;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }
            else {
                player.npc_state = NPC_TALKING; player.dialogue_step = 0; zone2_branch_chosen = false;
                player.state_timer = ComputeLineTimer(SCEN2_MAIN[0].text);
            }
        } else if (player.npc_state == NPC_TALKING) {
            shadow_flip = true;
            if (!zone2_branch_chosen && player.dialogue_step == SCEN2_MAIN_COUNT) {
                zone2_use_stick_branch = (stick.owner == STICK_PLAYER);
                zone2_branch_chosen = true;
            }
            int total_lines = Zone2TotalLines();
            const DialogueLine* line = GetZone2Line(player.dialogue_step);

            if (line != NULL && TextIsEqual(line->speaker, "action")) {
                if (action_timer == 0) {
                    action_timer = 30;
                    stick_anim_x = shadow_x - 4.0f;
                    stick_anim_y = player.y - 10.0f;
                }
                action_timer--;
                float t = 1.0f - (action_timer / 30.0f);
                float sx0 = shadow_x - 4.0f, sy0 = player.y - 10.0f;
                stick_anim_x = sx0 + (SHOE_X - sx0) * t;
                stick_anim_y = sy0 + (WIRE_Y - sy0) * t;
                if (action_timer <= 0) {
                    shoe_retrieved = true;
                    stick.active = false;
                    player.dialogue_step++;
                    const DialogueLine* next = GetZone2Line(player.dialogue_step);
                    if (next != NULL) player.state_timer = ComputeLineTimer(next->text);
                }
            } else if (line != NULL) {
                player.state_timer--;
                if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                    player.dialogue_step++;
                    const DialogueLine* next = GetZone2Line(player.dialogue_step);
                    if (next != NULL) player.state_timer = ComputeLineTimer(next->text);
                }
            }

            if (player.dialogue_step >= total_lines) {
                player.npc_state = NPC_WAVING; shadow_npcs[2].waving = true;
                shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 4) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[3].x + 16.0f;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }
            else { player.npc_state = NPC_TALKING; player.dialogue_step = 0; player.state_timer = ComputeLineTimer(zone4_lines[0]); }
        } else if (player.npc_state == NPC_TALKING) {
            shadow_flip = true; player.state_timer--;
            if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                player.dialogue_step++;
                if (player.dialogue_step < zone4_line_count) {
                    player.state_timer = ComputeLineTimer(zone4_lines[player.dialogue_step]);
                } else {
                    player.npc_state = NPC_WAVING; shadow_npcs[3].waving = true;
                    shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
                }
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 5) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[4].x + 16.0f;
            float prev_x = shadow_x;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }

            bool crossed_light = (prev_x < ZONE5_LIGHT_X && shadow_x >= ZONE5_LIGHT_X);
            if (crossed_light) {
                zone5_zapped = true; shadow_x = ZONE5_LIGHT_X;
                player.npc_state = NPC_TALKING; player.dialogue_step = 0;
                player.state_timer = ComputeLineTimer("Hey, ooh, ouch!");
            } else if (shadow_x == target) {
                zone5_zapped = false;
                player.npc_state = NPC_TALKING; player.dialogue_step = 0;
                player.state_timer = ComputeLineTimer(ZONE5_CONVO[0].text);
            }
        } else if (player.npc_state == NPC_TALKING) {
            shadow_flip = true;
            if (zone5_zapped) {
                player.state_timer--;
                if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                    scenarios_completed++; ResetShadowTakeover();
                }
            } else {
                player.state_timer--;
                if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                    player.dialogue_step++;
                    if (player.dialogue_step < ZONE5_CONVO_COUNT) {
                        player.state_timer = ComputeLineTimer(ZONE5_CONVO[player.dialogue_step].text);
                    } else {
                        player.npc_state = NPC_WAVING; shadow_npcs[4].waving = true;
                        shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
                    }
                }
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 6) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[5].x + 16.0f;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }
            else { player.npc_state = NPC_TALKING; player.dialogue_step = 0; player.state_timer = ComputeLineTimer(ZONE6_CONVO[0].text); }
        } else if (player.npc_state == NPC_TALKING) {
            shadow_flip = true; player.state_timer--;
            if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                player.dialogue_step++;
                if (player.dialogue_step < ZONE6_CONVO_COUNT) {
                    player.state_timer = ComputeLineTimer(ZONE6_CONVO[player.dialogue_step].text);
                } else {
                    player.npc_state = NPC_WAVING; shadow_npcs[5].waving = true;
                    shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
                    completed_zone6 = true;
                }
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 7) {
        if (player.npc_state == NPC_TALKING) {
            player.state_timer--;
            if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                player.dialogue_step++;
                if (player.dialogue_step < ZONE7_WATER_COUNT) {
                    player.state_timer = ComputeLineTimer(ZONE7_WATER[player.dialogue_step].text);
                } else {
                    shadow_npcs[7].state = SHADOW_HIDDEN;
                    scenarios_completed++;
                    ResetShadowTakeover();
                }
            }
        }
        return;
    }

    if (current_zone != NULL && current_zone->id == 8) {
        if (player.npc_state == NPC_WALK) {
            float target = shadow_npcs[6].x + 16.0f;
            if (shadow_x < target) { shadow_x += 0.8f; if (shadow_x > target) shadow_x = target; shadow_flip = false; }
            else if (shadow_x > target) { shadow_x -= 0.8f; if (shadow_x < target) shadow_x = target; shadow_flip = true; }
            else { player.npc_state = NPC_TALKING; player.dialogue_step = 0; player.state_timer = ComputeLineTimer(ZONE8_CONVO[0].text); }
        } else if (player.npc_state == NPC_TALKING) {
            shadow_flip = true; player.state_timer--;
            if (player.state_timer <= 0 || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                player.dialogue_step++;
                if (player.dialogue_step < ZONE8_CONVO_COUNT) {
                    player.state_timer = ComputeLineTimer(ZONE8_CONVO[player.dialogue_step].text);
                } else {
                    player.npc_state = NPC_WAVING; shadow_npcs[6].waving = true;
                    shadow_returning = true; scenarios_completed++; mode = MODE_PLAY;
                }
            }
        }
        return;
    }

    if (player.npc_state == NPC_STEAL_STICK) {
        if (shadow_x < player.x) { shadow_x += 1; shadow_flip = false; }
        else if (shadow_x > player.x) { shadow_x -= 1; shadow_flip = true; }
        else {
            stick.owner = STICK_SHADOW;
            player.npc_state = NPC_WALK;
            player.state_timer = 40 + (int)pico_rnd(40);
            player.target_x = player.flip ? current_zone->start_x : current_zone->end_x;
        }
    } else if (player.npc_state == NPC_WALK) {
        player.state_timer--;
        if (shadow_x < player.target_x) { shadow_x += 1; shadow_flip = false; }
        else if (shadow_x > player.target_x) { shadow_x -= 1; shadow_flip = true; }
        if (player.state_timer <= 0 && shadow_x != player.target_x) {
            player.npc_state = NPC_LOOK; player.state_timer = 45 + (int)pico_rnd(45);
        }
    } else if (player.npc_state == NPC_LOOK) {
        player.state_timer--;
        if (player.state_timer % 15 == 0 && pico_rnd(1.0f) < 0.2f) shadow_flip = !shadow_flip;
        if (player.state_timer <= 0) { player.npc_state = NPC_WALK; player.state_timer = 30 + (int)pico_rnd(60); }
    }
}

void UpdateShadowReturn(void) {
    if (!shadow_returning) return;
    float dx = player.x - shadow_x;
    if (fabsf(dx) > 1.5f) {
        shadow_x += (dx > 0.0f) ? 1.4f : -1.4f;
        shadow_flip = (dx < 0.0f);
    } else {
        shadow_x = player.x;
        shadow_returning = false;
        ResetShadowTakeover();
    }
}

void UpdateShadowChase(void) {
    if (!shadow_chasing) return;
    float dx = player.x - shadow_x;
    float dist = fabsf(dx);
    if (dist > 2.0f) {
        float chase_speed = 1.5f;
        shadow_x += (dx > 0.0f) ? chase_speed : -chase_speed;
        shadow_flip = (dx < 0.0f);
    } else {
        shadow_chasing = false;
        ResetShadowTakeover();
    }
}

void UpdateStickPhysics(void) {
    if (stick.is_thrown) {
        stick.x += stick.h_speed;
        if (fabsf(stick.x - player.x) > 150) {
            stick.is_thrown = false; stick.h_speed = 0;
            stick.x = player.x + 40; stick.y = 96; stick.owner = STICK_WORLD;
        }
    }
}

void UpdateCars(void) {
    float mid_cam_x = cam_x * 0.60f;
    if (player.is_walking) {
        car_timer++;
        if (car_timer >= 150) {
            car_timer = 0;
            bool dr = pico_rnd(1.0f) > 0.5f;
            float sx1 = dr ? (mid_cam_x - 30) : (mid_cam_x + 150);
            float sp1 = dr ? 1.2f : -1.2f;
            SpawnVehicle(sx1, 96, sp1, !dr, 0);

            bool dr2 = !dr;
            float sx2 = dr2 ? (mid_cam_x - 30) : (mid_cam_x + 150);
            float sp2 = dr2 ? 0.6f : -0.6f;
            SpawnVehicle(sx2, 96, sp2, !dr2, 0);

            SpawnVehicle(sx1 - (dr ? 15.0f : -15.0f), 96, sp1, !dr, 1);
        }
    }
    for (int i = 0; i < MAX_CARS; i++) {
        if (cars[i].active) {
            cars[i].x += cars[i].speed;
            if (cars[i].x < mid_cam_x - 50 || cars[i].x > mid_cam_x + 180) cars[i].active = false;
        }
    }
}

// ============================================================================
// DRAWING
// ============================================================================
void DrawPixelBitmap(const int bitmap[8][8], float x, float y, bool flip) {
    for (int py = 0; py < 8; py++) {
        for (int px = 0; px < 8; px++) {
            int c = bitmap[py][px];
            if (c > 0) {
                int dx = flip ? (int)(x + (7 - px)) : (int)(x + px);
                DrawPixel(dx, (int)(y + py), P8_SPRITE_PALETTE[c]);
            }
        }
    }
}

void DrawPixelBitmapVFlip(const int bitmap[8][8], float x, float y, bool flip_h) {
    for (int py = 0; py < 8; py++) {
        for (int px = 0; px < 8; px++) {
            int c = bitmap[7 - py][px];
            if (c > 0) {
                int dx = flip_h ? (int)(x + (7 - px)) : (int)(x + px);
                DrawPixel(dx, (int)(y + py), P8_SPRITE_PALETTE[c]);
            }
        }
    }
}

void DrawShoe(float x, float y) {
    DrawRectangle((int)x - 2, (int)y, 4, 2, P8_BROWN);
}

void DrawCigaretteSmoke(float x, float y, bool flip) {
    int cx = flip ? (int)(x + 1) : (int)(x + 6);
    DrawPixel(cx, (int)(y + 2), P8_RED);
    DrawPixel(cx, (int)(y - 1), P8_DARK_GRAY);
    DrawPixel(cx + (flip ? -1 : 1), (int)(y - 3), P8_DARK_GRAY);
    DrawPixel(cx, (int)(y - 5), P8_DARK_GRAY);
}

void DrawVehicle(float x, float y, int type) {
    Color body = (type == 1) ? P8_WHITE : P8_RED;
    DrawRectangle((int)x, (int)y, 5, 2, body);
    DrawRectangle((int)x + 1, (int)y - 1, 3, 1, body);
    DrawPixel((int)x, (int)y + 2, P8_BLACK);
    DrawPixel((int)x + 4, (int)y + 2, P8_BLACK);
    if (type == 1) {
        DrawPixel((int)x + 2, (int)y, P8_RED);
        DrawPixel((int)x + 1, (int)y + 1, P8_RED);
        DrawPixel((int)x + 3, (int)y + 1, P8_RED);
    }
}

#define MAX_BUBBLE_LINES 4
#define MAX_LINE_LEN 64

int WrapDialogueText(const char* text, int font_size, int max_width, char lines[][MAX_LINE_LEN]) {
    char buffer[320];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    int line_count = 0;
    char current_line[MAX_LINE_LEN] = "";
    char* word = strtok(buffer, " ");
    while (word != NULL && line_count < MAX_BUBBLE_LINES) {
        char candidate[MAX_LINE_LEN];
        if (current_line[0] == '\0') snprintf(candidate, MAX_LINE_LEN, "%s", word);
        else snprintf(candidate, MAX_LINE_LEN, "%s %s", current_line, word);

        if (current_line[0] == '\0' || MeasureText(candidate, font_size) <= max_width) {
            strncpy(current_line, candidate, MAX_LINE_LEN - 1); current_line[MAX_LINE_LEN - 1] = '\0';
        } else {
            strncpy(lines[line_count], current_line, MAX_LINE_LEN - 1); lines[line_count][MAX_LINE_LEN - 1] = '\0';
            line_count++;
            strncpy(current_line, word, MAX_LINE_LEN - 1); current_line[MAX_LINE_LEN - 1] = '\0';
        }
        word = strtok(NULL, " ");
    }
    if (current_line[0] != '\0' && line_count < MAX_BUBBLE_LINES) {
        strncpy(lines[line_count], current_line, MAX_LINE_LEN - 1); lines[line_count][MAX_LINE_LEN - 1] = '\0';
        line_count++;
    }
    return line_count;
}

Vector2 WorldToScreen(float wx, float wy) {
    float sx = ((wx - focus_x) * camera_zoom + SCREEN_WIDTH / 2.0f) * SCALE_FACTOR;
    float sy = ((wy - focus_y) * camera_zoom + SCREEN_HEIGHT / 2.0f) * SCALE_FACTOR;
    return (Vector2){ sx, sy };
}

void DrawHintTextScreen(const char* text, float wx, float wy) {
    int font_size = (int)(22.0f * camera_zoom);
    if (font_size < 18) font_size = 18;
    Vector2 sp = WorldToScreen(wx, wy);
    int tw = MeasureText(text, font_size);
    DrawRectangle((int)sp.x - tw / 2 - 4, (int)sp.y - 3, tw + 8, font_size + 6, Fade(P8_BLACK, 0.4f));
    DrawText(text, (int)sp.x - tw / 2, (int)sp.y, font_size, P8_WHITE);
}

void DrawSpeechBubbleScreenEx(const char* text, float speaker_wx, float speaker_wy_anchor, bool point_left, bool below) {
    int font_size = (int)(12.0f * camera_zoom);
    if (font_size < 10) font_size = 10;
    if (font_size > 30) font_size = 30;
    int padding_x = 8, padding_y = 4, line_gap = 3, max_text_w = 340;

    char lines[MAX_BUBBLE_LINES][MAX_LINE_LEN];
    int line_count = WrapDialogueText(text, font_size, max_text_w, lines);
    if (line_count < 1) line_count = 1;

    int text_w = 0;
    for (int i = 0; i < line_count; i++) { int w = MeasureText(lines[i], font_size); if (w > text_w) text_w = w; }

    int bubble_w = text_w + padding_x * 2;
    int bubble_h = font_size * line_count + line_gap * (line_count - 1) + padding_y * 2;

    Vector2 sp = WorldToScreen(speaker_wx, speaker_wy_anchor);

    int bx;
    if (point_left) bx = (int)sp.x - bubble_w + 10;
    else bx = (int)sp.x - 10;

    int by = below ? ((int)sp.y + 12) : ((int)sp.y - bubble_h - 12);

    int win_w = SCREEN_WIDTH * SCALE_FACTOR, win_h = SCREEN_HEIGHT * SCALE_FACTOR;
    if (bx < 4) bx = 4;
    if (bx + bubble_w > win_w - 4) bx = win_w - 4 - bubble_w;
    if (by < 4) by = 4;
    if (by > win_h - bubble_h - 4) by = win_h - bubble_h - 4;

    DrawRectangle(bx, by, bubble_w, bubble_h, P8_WHITE);
    DrawRectangleLines(bx - 2, by - 2, bubble_w + 4, bubble_h + 4, P8_BLACK);

    int tx = point_left ? (bx + bubble_w - 8) : (bx + 8);
    if (below) {
        int ty = by;
        DrawTriangle((Vector2){ (float)tx - 5, (float)ty }, (Vector2){ (float)tx + 5, (float)ty }, (Vector2){ (float)tx, (float)ty - 9 }, P8_WHITE);
    } else {
        int ty = by + bubble_h;
        DrawTriangle((Vector2){ (float)tx - 5, (float)ty }, (Vector2){ (float)tx + 5, (float)ty }, (Vector2){ (float)tx, (float)ty + 9 }, P8_WHITE);
    }

    for (int i = 0; i < line_count; i++) {
        int lw = MeasureText(lines[i], font_size);
        int lx = bx + (bubble_w - lw) / 2;
        int ly = by + padding_y + i * (font_size + line_gap);
        DrawText(lines[i], lx, ly, font_size, P8_BLACK);
    }
}

void DrawDialogueBubbleScreen(const char* text, float speaker_wx, float speaker_wy_top, float other_wx) {
    DrawSpeechBubbleScreenEx(text, speaker_wx, speaker_wy_top - 8, speaker_wx > other_wx, false);
}

void DrawDialogueBubbleScreenBelow(const char* text, float speaker_wx, float speaker_wy_bottom, float other_wx) {
    DrawSpeechBubbleScreenEx(text, speaker_wx, speaker_wy_bottom, speaker_wx > other_wx, true);
}

void DrawGameSprite(int id, float x, float y, bool flip, Color tint) {
    if (id == 16) DrawPixelBitmap(SPRITE_PLAYER_SHADOW_IDLE, x, y, flip);
    else if (id == 17) DrawPixelBitmap(SPRITE_PLAYER_SHADOW_WAVE, x, y, flip);
    else if (id == 24) DrawPixelBitmap(SPRITE_PLAYER_SHADOW_WALK_B, x, y, flip);
    else if (id == 116) DrawPixelBitmap(SPRITE_NPC_SHADOW_IDLE, x, y, flip);
    else if (id == 117) DrawPixelBitmap(SPRITE_NPC_SHADOW_WAVE, x, y, flip);
    else if (id == 4) DrawRectangle((int)x, (int)y, 8, 2, P8_BROWN);
    else if (id == 20) DrawPixelBitmap(SPRITE_PLAYER_WALK_B, x, y, flip);
    else DrawPixelBitmap(SPRITE_PLAYER_IDLE, x, y, flip);
}

void DrawCityLayer(Building *buildings, int count, float ground_y, bool is_far_layer, float layer_width, float current_cam_x) {
    float offset = floorf(current_cam_x / layer_width) * layer_width;
    for (float loop_shift = -layer_width; loop_shift <= layer_width; loop_shift += layer_width) {
        for (int i = 0; i < count; i++) {
            Building b = buildings[i];
            float render_x = b.x + offset + loop_shift;
            float roof_y = ground_y - b.h;

            if (!is_far_layer) {
                float delta = (roof_y - 66) - cam_y;
                if (delta > 0) {
                    float shadow_top = roof_y - 1;
                    if (delta < 5) {
                        for (int px = 0; px < b.w; px++)
                            if ((px % 2) == 0) DrawPixel((int)(render_x + px), (int)(roof_y - 1), P8_DARK_BLUE);
                    } else {
                        shadow_top = roof_y - 2;
                        DrawRectangle((int)render_x, (int)(roof_y - 2), (int)b.w, 2, P8_DARK_BLUE);
                    }
                    DrawRectangle((int)(render_x - 1), (int)shadow_top, 1, (int)(ground_y - shadow_top), P8_DARK_GRAY);
                } else {
                    DrawRectangle((int)(render_x - 1), (int)roof_y, 1, (int)b.h, P8_DARK_GRAY);
                }
                DrawRectangle((int)(render_x - 1), (int)(ground_y + 1), (int)(b.w + 1), 1, P8_DARK_GRAY);
            }

            DrawRectangle((int)render_x, (int)roof_y, (int)b.w, (int)b.h, b.color);

            int x_step = is_far_layer ? 3 : 4;
            int y_step = is_far_layer ? 4 : 5;
            Color win_color = is_far_layer ? P8_DARK_BLUE : P8_GREEN;
            pico_srand((unsigned int)(b.x * 100));
            for (float wx = render_x + 2; wx <= render_x + b.w - 2; wx += x_step) {
                for (float wy = roof_y + 3; wy <= ground_y - 4; wy += y_step) {
                    if (pico_rnd(1.0f) < window_lit_chance) {
                        if (is_far_layer) DrawPixel((int)wx, (int)wy, win_color);
                        else DrawRectangle((int)wx, (int)wy, 1, 2, win_color);
                    }
                }
            }
        }
    }
}

void DrawBoldTextScreen(const char* text, int cx, int y, int font_size, Color color) {
    int tw = MeasureText(text, font_size);
    int x = cx - tw / 2;
    int offs[5][2] = {{0,0},{-1,0},{1,0},{0,-1},{0,1}};
    for (int i = 0; i < 5; i++) DrawText(text, x + offs[i][0], y + offs[i][1], font_size, color);
}

void DrawBoldTextLeft(const char* text, int x, int y, int font_size, Color color) {
    int offs[5][2] = {{0,0},{-1,0},{1,0},{0,-1},{0,1}};
    for (int i = 0; i < 5; i++) DrawText(text, x + offs[i][0], y + offs[i][1], font_size, color);
}

// ============================================================================
// MAIN
// ============================================================================
int main(void) {
    const int windowWidth = SCREEN_WIDTH * SCALE_FACTOR;
    const int windowHeight = SCREEN_HEIGHT * SCALE_FACTOR;

    InitWindow(windowWidth, windowHeight, "Shadow Takeover");
	
	Image icon = LoadImage("icon.png"); 
	SetWindowIcon(icon);
	UnloadImage(icon);
	
    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_STREAM_BUFFER_SIZE);
    AudioStream audio_stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 32, 1);
    SetAudioStreamCallback(audio_stream, AudioMashupCallback);
    PlayAudioStream(audio_stream);

    SetTargetFPS(TARGET_FPS);

    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    player = (Player){ .x = 64, .y = 100, .ground_y = 100, .dy = 0, .is_jumping = false, .speed = 1.75f,
        .flip = false, .is_walking = false, .npc_state = NPC_WALK, .state_timer = 0, .dialogue_step = 0 };

    stick = (Stick){ .x = (szones[0].end_x + szones[1].start_x) / 2.0f, .y = 96, .active = true,
        .is_thrown = false, .h_speed = 0, .throw_flip = false, .owner = STICK_WORLD };

    for (int i = 0; i < NUM_ZONES; i++) {
        shadow_npcs[i] = (ShadowNPC){
            .x = szones[i].start_x + floorf((szones[i].end_x - szones[i].start_x) / 2.0f),
            .y = 100, .zone = szones[i], .active = true, .flip = false, .state = SHADOW_HIDDEN, .waving = false
        };
    }
    shadow_npcs[7] = (ShadowNPC){ .x = 0, .y = 100, .zone = water_zone, .active = true, .flip = false, .state = SHADOW_HIDDEN, .waving = false };

    pico_srand(42);
    for (int i = 0; i < 30; i++) {
        skyline_far[i] = (Building){ .x = i * 16 + floorf(pico_rnd(6)), .w = 8 + floorf(pico_rnd(10)), .h = 35 + floorf(pico_rnd(30)), .color = P8_DARK_PURP };
    }

    pico_srand(777);
    for (int i = 0; i < 15; i++) {
        skyline_blue[i] = (Building){ .x = i * 32 + floorf(pico_rnd(10)), .w = 14 + floorf(pico_rnd(14)), .h = 40 + floorf(pico_rnd(30)), .color = P8_BUILDING_BLUE };
    }

    pico_srand(1337);
    for (int i = 0; i < 20; i++) {
        skyline_mid[i] = (Building){ .x = i * 24 + floorf(pico_rnd(8)), .w = 16 + floorf(pico_rnd(12)), .h = 50 + floorf(pico_rnd(35)), .color = P8_DARK_GREEN };
    }

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

    while (!WindowShouldClose()) {

        if (app_screen == SCREEN_TITLE || app_screen == SCREEN_END) {
            title_scroll += 0.4f;
            title_car_timer++;
            if (title_car_timer >= 90) {
                title_car_timer = 0;
                bool dr = pico_rnd(1.0f) > 0.5f;
                float sx1 = dr ? (title_scroll * 0.60f - 30) : (title_scroll * 0.60f + 150);
                SpawnVehicle(sx1, 96, dr ? 1.2f : -1.2f, !dr, (pico_rnd(1.0f) < 0.25f) ? 1 : 0);
            }
            for (int i = 0; i < MAX_CARS; i++) {
                if (cars[i].active) {
                    cars[i].x += cars[i].speed;
                    if (cars[i].x < title_scroll * 0.60f - 50 || cars[i].x > title_scroll * 0.60f + 180) cars[i].active = false;
                }
            }

            window_lit_chance = 0.25f + 0.15f * sinf((float)GetTime() * 0.35f);

            BeginTextureMode(target);
                ClearBackground(P8_TITLE_BG);
                camera.zoom = 1.0f;

                camera.target = (Vector2){ title_scroll * 0.25f, 0 };
                BeginMode2D(camera);
                    DrawCityLayer(skyline_far, 30, 100, true, far_layer_width, camera.target.x);
                EndMode2D();

                camera.target = (Vector2){ title_scroll * 0.40f, 0 };
                BeginMode2D(camera);
                    DrawCityLayer(skyline_blue, 15, 100, true, blue_layer_width, camera.target.x);
                EndMode2D();

                camera.target = (Vector2){ title_scroll * 0.60f, 0 };
                BeginMode2D(camera);
                    DrawCityLayer(skyline_mid, 20, 100, false, mid_layer_width, camera.target.x);
                    for (int i = 0; i < MAX_CARS; i++)
                        if (cars[i].active) DrawVehicle(cars[i].x, cars[i].y - 4, cars[i].type);
                EndMode2D();

                camera.target = (Vector2){ 0, 0 };
                BeginMode2D(camera);
                    DrawRectangle((int)(title_scroll) - 10, 100, 148, 100, P8_GROUND_GREEN);
                EndMode2D();
            EndTextureMode();

            BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(target.texture,
                    (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
                    (Rectangle){ 0.0f, 0.0f, (float)windowWidth, (float)windowHeight },
                    (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

                if (app_screen == SCREEN_TITLE) {
                    int bigS = 72 * 3;
                    int restSz = 72;
                    int gap = 16;
                    int sW = MeasureText("S", bigS);
                    int restW = MeasureText("AYAHA", restSz);
                    int totalW = sW + gap + restW;
                    int startX = windowWidth / 2 - totalW / 2;
                    int baseY = windowHeight / 2 - 100;
                    DrawBoldTextLeft("S", startX, baseY - (bigS - restSz) / 2, bigS, P8_YELLOW);
                    DrawBoldTextLeft("AYAHA", startX + sW + gap, baseY, restSz, P8_YELLOW);

                    int subW = MeasureText("The shadow take over", 26);
                    DrawText("The shadow take over", windowWidth / 2 - subW / 2, windowHeight / 2 + 60, 26, P8_YELLOW);
                    if (((int)(GetTime() * 2.0) % 2) == 0) {
                        const char* prompt = "Press ENTER to start";
                        int pw = MeasureText(prompt, 20);
                        DrawText(prompt, windowWidth / 2 - pw / 2, windowHeight - 60, 20, P8_WHITE);
                    // ========================================================
                    // (Arrows & S Key)
                    // ========================================================
                    int ui_x = 40;
                    int ui_y = windowHeight - 110;
                    int box_size = 28;
                    int spacing = 4;

                    // Helper lambda/block or direct drawing for D-Pad / Arrows
                    // Up Arrow Box
                    DrawRectangle(ui_x + box_size + spacing, ui_y, box_size, box_size, Fade(P8_DARK_GRAY, 0.6f));
                    DrawRectangleLines(ui_x + box_size + spacing, ui_y, box_size, box_size, P8_WHITE);
                    DrawText("^", ui_x + box_size + spacing + 10, ui_y + 4, 20, P8_WHITE);

                    // Left Arrow Box
                    DrawRectangle(ui_x, ui_y + box_size + spacing, box_size, box_size, Fade(P8_DARK_GRAY, 0.6f));
                    DrawRectangleLines(ui_x, ui_y + box_size + spacing, box_size, box_size, P8_WHITE);
                    DrawText("<", ui_x + 8, ui_y + box_size + spacing + 4, 20, P8_WHITE);

                    // Down Arrow Box
                    DrawRectangle(ui_x + box_size + spacing, ui_y + box_size + spacing, box_size, box_size, Fade(P8_DARK_GRAY, 0.6f));
                    DrawRectangleLines(ui_x + box_size + spacing, ui_y + box_size + spacing, box_size, box_size, P8_WHITE);
                    DrawText("v", ui_x + box_size + spacing + 9, ui_y + box_size + spacing + 2, 20, P8_WHITE);

                    // Right Arrow Box
                    DrawRectangle(ui_x + (box_size + spacing) * 2, ui_y + box_size + spacing, box_size, box_size, Fade(P8_DARK_GRAY, 0.6f));
                    DrawRectangleLines(ui_x + (box_size + spacing) * 2, ui_y + box_size + spacing, box_size, box_size, P8_WHITE);
                    DrawText(">", ui_x + (box_size + spacing) * 2 + 8, ui_y + box_size + spacing + 4, 20, P8_WHITE);

                    // 'S' Key Box next to arrows
                    int s_key_x = ui_x + (box_size + spacing) * 3 + 12;
                    DrawRectangle(s_key_x, ui_y + box_size + spacing, box_size, box_size, Fade(P8_DARK_GRAY, 0.6f));
                    DrawRectangleLines(s_key_x, ui_y + box_size + spacing, box_size, box_size, P8_YELLOW);
                    DrawText("S", s_key_x + 8, ui_y + box_size + spacing + 4, 20, P8_YELLOW);

                    // Label text underneath
                    DrawText("MOVE / ACTION", ui_x, ui_y + (box_size * 2) + spacing * 2 + 4, 10, P8_WHITE);
					}
                } else {
                    DrawBoldTextScreen("THE END", windowWidth / 2, windowHeight / 2 - 40, 72, P8_YELLOW);
                    if (((int)(GetTime() * 2.0) % 2) == 0) {
                        const char* prompt = "Press ENTER to return to title";
                        int pw = MeasureText(prompt, 20);
                        DrawText(prompt, windowWidth / 2 - pw / 2, windowHeight - 60, 20, P8_WHITE);
                    }
                }
            EndDrawing();

            if (IsKeyPressed(KEY_ENTER)) {
                if (app_screen == SCREEN_TITLE) {
                    app_screen = SCREEN_GAME;
                } else {
                    app_screen = SCREEN_TITLE;
                    player.x = 64; player.y = 100; player.flip = false; player.is_jumping = false; player.dy = 0;
                    stick = (Stick){ .x = (szones[0].end_x + szones[1].start_x) / 2.0f, .y = 96, .active = true,
                        .is_thrown = false, .h_speed = 0, .throw_flip = false, .owner = STICK_WORLD };
                    shadow_detached = false; shadow_returning = false; shadow_chasing = false; mode = MODE_PLAY;
                    shoe_retrieved = false; completed_zone6 = false; scenarios_completed = 0;
                    zone4_total_visits = 0; zone4_side_switches = 0;
                    for (int i = 0; i < 8; i++) { shadow_npcs[i].waving = false; shadow_npcs[i].state = SHADOW_HIDDEN; }
                }
                for (int i = 0; i < MAX_CARS; i++) cars[i].active = false;
                car_timer = 0;
            }
            continue;
        }

        CheckShadowZones();
        bool s_pressed = IsKeyPressed(KEY_S);

        if (mode == MODE_PLAY) UpdatePlayerControl(s_pressed);
        else if (mode == MODE_WALK_TO_WALL) UpdateWalkToWall();
        else if (mode == MODE_SHADOW_TAKEOVER) UpdateShadowNPC(s_pressed);

        UpdateShadowReturn();
        UpdateShadowChase();
        UpdateStickPhysics();
        UpdateCars();
        UpdateShadowNPCs();

        bool visually_in_shadow = false;
        for (int i = 0; i < NUM_ZONES; i++) {
            if (player.x >= szones[i].start_x && player.x <= szones[i].end_x) { visually_in_shadow = true; break; }
        }
        bool player_on_water = (player.x >= water_zone.start_x && player.x <= water_zone.end_x);

        float progress = (float)scenarios_completed / (float)TOTAL_SCENARIOS;
        if (progress > 1.0f) progress = 1.0f;
        window_lit_chance = 0.4f * (1.0f - progress);

        cam_x = player.x - 64.0f;
        cam_y = player.y - 90.0f;

        bool shadow_active = (mode == MODE_SHADOW_TAKEOVER && shadow_detached);
        camera_zoom_target = shadow_active ? 1.6f : 1.0f;

        float dialogue_npc_x = shadow_npcs[0].x;
        if (current_zone != NULL) {
            if (current_zone->id == 2) dialogue_npc_x = shadow_npcs[1].x;
            else if (current_zone->id == 3) dialogue_npc_x = shadow_npcs[2].x;
            else if (current_zone->id == 4) dialogue_npc_x = shadow_npcs[3].x;
            else if (current_zone->id == 5) dialogue_npc_x = shadow_npcs[4].x;
            else if (current_zone->id == 6) dialogue_npc_x = shadow_npcs[5].x;
            else if (current_zone->id == 7) dialogue_npc_x = shadow_npcs[7].x;
            else if (current_zone->id == 8) dialogue_npc_x = shadow_npcs[6].x;
        }
        float target_focus_x = shadow_active ? (dialogue_npc_x + shadow_x) * 0.5f : (cam_x + SCREEN_WIDTH / 2.0f);

        bool in_water_zone7 = (current_zone != NULL && current_zone->id == 7);
        float target_focus_y = shadow_active
            ? (in_water_zone7 ? 106.0f : (player.y - 40.0f))
            : (cam_y + SCREEN_HEIGHT / 2.0f);

        if (shadow_active) {
            float lerp_speed = 0.06f;
            camera_zoom += (camera_zoom_target - camera_zoom) * lerp_speed;
            focus_x += (target_focus_x - focus_x) * lerp_speed;
            focus_y += (target_focus_y - focus_y) * lerp_speed;
        } else {
            camera_zoom = 1.0f;
            focus_x = target_focus_x;
            focus_y = target_focus_y;
        }

        BeginTextureMode(target);
            ClearBackground(P8_DARK_BLUE);
            camera.zoom = camera_zoom;

            camera.target = (Vector2){ focus_x * 0.25f, focus_y * 0.25f };
            BeginMode2D(camera);
                DrawCityLayer(skyline_far, 30, 100, true, far_layer_width, camera.target.x);
            EndMode2D();

            camera.target = (Vector2){ focus_x * 0.40f, focus_y * 0.40f };
            BeginMode2D(camera);
                DrawCityLayer(skyline_blue, 15, 100, true, blue_layer_width, camera.target.x);
            EndMode2D();

            camera.target = (Vector2){ focus_x * 0.60f, focus_y * 0.60f };
            BeginMode2D(camera);
                DrawCityLayer(skyline_mid, 20, 100, false, mid_layer_width, camera.target.x);
                for (int i = 0; i < MAX_CARS; i++)
                    if (cars[i].active) DrawVehicle(cars[i].x, cars[i].y - 4, cars[i].type);
            EndMode2D();

            camera.target = (Vector2){ focus_x, focus_y };
            BeginMode2D(camera);
                DrawRectangle((int)cam_x - 10, 100, 148, 100, P8_GROUND_GREEN);

                for (int i = 0; i < NUM_ZONES; i++)
                    DrawRectangle((int)szones[i].start_x, -50, (int)(szones[i].end_x - szones[i].start_x), 150, P8_BLACK);

                {
                    float wmid = (water_zone.start_x + water_zone.end_x) / 2.0f;
                    float wradius = (water_zone.end_x - water_zone.start_x) / 2.0f;
                    DrawEllipse((int)wmid, 100, (int)wradius, 8, P8_WATER);
                    DrawRectangle((int)(wmid - wradius - 2), 85, (int)(wradius * 2.0f + 4.0f), 15, P8_GROUND_GREEN);
                    if (player_on_water && player.is_walking) {
                        ripple_timer += 1.0f;
                        float r = fmodf(ripple_timer, 24.0f);
                        DrawEllipseLines((int)player.x, 100, (int)(6.0f + r * 0.9f), (int)(2.0f + r * 0.25f), Fade(P8_WHITE, 1.0f - r / 24.0f));
                    } else {
                        ripple_timer = 0.0f;
                    }
                }

                DrawRectangle((int)WIRE_POLE1_X - 1, (int)WIRE_Y, 3, (int)(100 - WIRE_Y), P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE1_X - 6, (int)WIRE_Y, 12, 2, P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE1_X - 6, (int)WIRE_Y - 2, 1, 2, P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE1_X + 5, (int)WIRE_Y - 2, 1, 2, P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE2_X - 1, (int)WIRE_Y, 3, (int)(100 - WIRE_Y), P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE2_X - 6, (int)WIRE_Y, 12, 2, P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE2_X - 6, (int)WIRE_Y - 2, 1, 2, P8_POLE_DARK);
                DrawRectangle((int)WIRE_POLE2_X + 5, (int)WIRE_Y - 2, 1, 2, P8_POLE_DARK);
                DrawLine((int)WIRE_POLE1_X, (int)WIRE_Y + 1, (int)WIRE_POLE2_X, (int)WIRE_Y + 1, P8_POLE_DARK);
                if (!shoe_retrieved) DrawShoe(SHOE_X, WIRE_Y);

                {
                    float post_x = ZONE5_LIGHT_X - 15.0f;
                    DrawRectangle((int)post_x - 1, 22, 2, 78, P8_POLE_DARK);
                    DrawRectangle((int)post_x - 3, 18, 6, 4, P8_POLE_DARK);
                    Vector2 lamp1 = { post_x - 1, 22 };
                    Vector2 lamp2 = { post_x + 1, 22 };
                    Vector2 grnd1 = { ZONE5_LIGHT_X - 16, 100 };
                    Vector2 grnd2 = { ZONE5_LIGHT_X + 10, 100 };
                    DrawTriangle(lamp1, grnd1, grnd2, Fade(P8_YELLOW, 0.45f));
                    DrawTriangle(lamp1, grnd2, lamp2, Fade(P8_YELLOW, 0.45f));
                }

                for (int i = 0; i < 7; i++) {
                    if (shadow_npcs[i].state == SHADOW_LURKING) {
                        int sprite_id = shadow_npcs[i].waving ? 117 : 116;
                        DrawGameSprite(sprite_id, shadow_npcs[i].x - 4, shadow_npcs[i].y - 8, shadow_npcs[i].flip, P8_DARK_GRAY);
                        if (i == 2 && shoe_retrieved) {
                            float fx = shadow_npcs[i].flip ? (shadow_npcs[i].x - 4 + 1) : (shadow_npcs[i].x - 4 + 5);
                            DrawShoe(fx, shadow_npcs[i].y - 2);
                        }
                        if (i == 5) DrawCigaretteSmoke(shadow_npcs[i].x - 4, shadow_npcs[i].y - 8, shadow_npcs[i].flip);
                    }
                }
                if (shadow_npcs[7].state == SHADOW_LURKING) {
                    DrawPixelBitmapVFlip(SPRITE_NPC_SHADOW_IDLE, shadow_npcs[7].x - 4, 100.0f, shadow_npcs[7].flip);
                }

                if (stick.owner == STICK_WORLD && stick.active) {
                    bool f = stick.is_thrown ? stick.throw_flip : false;
                    DrawGameSprite(4, stick.x, stick.y - 4, f, P8_BROWN);
                }

                if (player.is_walking) player_walk_counter++; else player_walk_counter = 0;
                int player_frame = (player_walk_counter / 4) % 2;
                int sprite_to_draw = 1;
                if (!player.is_jumping && player.is_walking && player_frame == 1) sprite_to_draw = 20;

                if (stick.owner == STICK_PLAYER && stick.active) {
                    float sx = player.flip ? player.x - 8 : player.x;
                    DrawGameSprite(4, sx, player.y - 10, player.flip, P8_BROWN);
                }

                if (player_on_water && !(shadow_detached && current_zone != NULL && current_zone->id == 7)) {
                    float rx = player.flip ? (player.x + 4.0f) : (player.x - 4.0f);
                    DrawPixelBitmapVFlip(SPRITE_PLAYER_SHADOW_IDLE, rx - 7.0f, 100.0f, player.flip);
                } else if (visually_in_shadow) {
                    float asx = player.flip ? (player.x + 4.0f) : (player.x - 4.0f);
                    DrawGameSprite(16, asx - 4, player.y - 8, player.flip, P8_WHITE);
                }

                if (shadow_detached) {
                    bool zapped_hidden = (current_zone != NULL && current_zone->id == 5 && zone5_zapped);
                    bool in_water_convo = (current_zone != NULL && current_zone->id == 7);
                    if (!zapped_hidden && !in_water_convo) {
                        bool shadow_is_moving = (player.npc_state == NPC_WALK || player.npc_state == NPC_STEAL_STICK || shadow_returning || shadow_chasing);
                        if (shadow_is_moving) shadow_walk_counter++; else shadow_walk_counter = 0;
                        int shadow_frame = (shadow_walk_counter / 4) % 2;

                        int detached_sprite;
                        if (player.npc_state == NPC_WAVING) detached_sprite = 17;
                        else if (shadow_is_moving && shadow_frame == 1) detached_sprite = 24;
                        else detached_sprite = 16;

                        DrawGameSprite(detached_sprite, shadow_x - 4, player.y - 8, shadow_flip, P8_WHITE);
                    } else if (in_water_convo) {
                        DrawPixelBitmapVFlip(SPRITE_PLAYER_SHADOW_IDLE, shadow_x - 7.0f, 100.0f, shadow_flip);
                    }

                    if (mode == MODE_SHADOW_TAKEOVER && current_zone != NULL && current_zone->id == 3 && player.npc_state == NPC_TALKING) {
                        const DialogueLine* cl = GetZone2Line(player.dialogue_step);
                        if (cl != NULL && TextIsEqual(cl->speaker, "action") && action_timer > 0)
                            DrawGameSprite(4, stick_anim_x, stick_anim_y, false, P8_BROWN);
                    }

                    if (stick.owner == STICK_SHADOW) {
                        float sx = shadow_flip ? shadow_x - 8 : shadow_x;
                        DrawGameSprite(4, sx, player.y - 10, shadow_flip, P8_WHITE);
                    }
                }

                DrawGameSprite(sprite_to_draw, player.x - 4, player.y - 8, player.flip, P8_WHITE);

            EndMode2D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(target.texture,
                (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
                (Rectangle){ 0.0f, 0.0f, (float)windowWidth, (float)windowHeight },
                (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

            if (mode == MODE_PLAY && stick.owner != STICK_PLAYER && stick.active && !stick.is_thrown && fabsf(player.x - stick.x) < 10)
                DrawHintTextScreen("DOWN/S to take", stick.x, stick.y - 28);

            if (visually_in_shadow && !shadow_detached && mode == MODE_PLAY && !player_on_water)
                DrawHintTextScreen("S: Shadow Takeover", player.x, player.y - 32);

            if (shadow_chasing && player_on_water)
                DrawHintTextScreen("S: Talk to the reflection", player.x, player.y - 32);

            if (shadow_detached && mode == MODE_SHADOW_TAKEOVER && player.npc_state == NPC_TALKING && current_zone != NULL) {
                if (current_zone->id == 1 && player.dialogue_step < ZONE_INTRO_COUNT) {
                    DialogueLine cl = ZONE_INTRO[player.dialogue_step];
                    if (TextIsEqual(cl.speaker, "npc")) DrawDialogueBubbleScreen(cl.text, shadow_npcs[0].x, shadow_npcs[0].y, shadow_x);
                    else DrawDialogueBubbleScreen(cl.text, shadow_x, player.y, shadow_npcs[0].x);
                } else if (current_zone->id == 2) {
                    int cnt; const DialogueLine* cl = GetZone2CigLine(player.dialogue_step, &cnt);
                    if (cl != NULL) {
                        if (TextIsEqual(cl->speaker, "npc")) DrawDialogueBubbleScreen(cl->text, shadow_npcs[1].x, shadow_npcs[1].y, shadow_x);
                        else DrawDialogueBubbleScreen(cl->text, shadow_x, player.y, shadow_npcs[1].x);
                    }
                } else if (current_zone->id == 3) {
                    const DialogueLine* cl = GetZone2Line(player.dialogue_step);
                    if (cl != NULL && !TextIsEqual(cl->speaker, "action")) {
                        if (TextIsEqual(cl->speaker, "npc")) DrawDialogueBubbleScreen(cl->text, shadow_npcs[2].x, shadow_npcs[2].y, shadow_x);
                        else DrawDialogueBubbleScreen(cl->text, shadow_x, player.y, shadow_npcs[2].x);
                    }
                } else if (current_zone->id == 4 && player.dialogue_step < zone4_line_count) {
                    DrawDialogueBubbleScreen(zone4_lines[player.dialogue_step], shadow_npcs[3].x, shadow_npcs[3].y, shadow_x);
                } else if (current_zone->id == 5) {
                    if (zone5_zapped) {
                        DrawDialogueBubbleScreen("Hey, ooh, ouch!", shadow_npcs[4].x, shadow_npcs[4].y, shadow_x);
                    } else if (player.dialogue_step < ZONE5_CONVO_COUNT) {
                        DialogueLine cl = ZONE5_CONVO[player.dialogue_step];
                        if (TextIsEqual(cl.speaker, "npc")) DrawDialogueBubbleScreen(cl.text, shadow_npcs[4].x, shadow_npcs[4].y, shadow_x);
                        else DrawDialogueBubbleScreen(cl.text, shadow_x, player.y, shadow_npcs[4].x);
                    }
                } else if (current_zone->id == 6 && player.dialogue_step < ZONE6_CONVO_COUNT) {
                    DialogueLine cl = ZONE6_CONVO[player.dialogue_step];
                    if (TextIsEqual(cl.speaker, "npc")) DrawDialogueBubbleScreen(cl.text, shadow_npcs[5].x, shadow_npcs[5].y, shadow_x);
                    else DrawDialogueBubbleScreen(cl.text, shadow_x, player.y, shadow_npcs[5].x);
                } else if (current_zone->id == 7 && player.dialogue_step < ZONE7_WATER_COUNT) {
                    DialogueLine cl = ZONE7_WATER[player.dialogue_step];
                    if (TextIsEqual(cl.speaker, "npc")) DrawDialogueBubbleScreenBelow(cl.text, shadow_npcs[7].x, 112.0f, shadow_x);
                    else DrawDialogueBubbleScreenBelow(cl.text, shadow_x, 112.0f, shadow_npcs[7].x);
                } else if (current_zone->id == 8 && player.dialogue_step < ZONE8_CONVO_COUNT) {
                    DialogueLine cl = ZONE8_CONVO[player.dialogue_step];
                    if (TextIsEqual(cl.speaker, "npc")) DrawDialogueBubbleScreen(cl.text, shadow_npcs[6].x, shadow_npcs[6].y, shadow_x);
                    else DrawDialogueBubbleScreen(cl.text, shadow_x, player.y, shadow_npcs[6].x);
                }
            }
        EndDrawing();
    }

    UnloadAudioStream(audio_stream);
    CloseAudioDevice();
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}