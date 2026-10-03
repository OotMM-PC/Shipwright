#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct Actor;
struct PlayState;

int OotmmXflags_IsShuffled(struct Actor* actor);
int OotmmXflags_Drop(struct Actor* actor, struct PlayState* play);
int OotmmXflags_DropStill(struct Actor* actor, struct PlayState* play);
int OotmmXflags_DropSlice(struct Actor* actor, struct PlayState* play, int slice, float x, float y, float z);
int OotmmXflags_Collect(struct Actor* actor);
void OotmmXflags_TagChild(struct Actor* child, struct Actor* parent, int slice);
int OotmmXflags_IsAnySliceShuffled(struct Actor* actor, int count);
void OotmmXflags_PrepareSpawn(struct Actor* parent, int slice);
void OotmmXflags_PrepareSpawnAt(int scene, int setup, int room, int id);
void OotmmXflags_ClearSpawn(void);
void OotmmXflags_ClaimSpawn(struct Actor* actor);
int OotmmXflags_PrepareFairy(struct Actor* butterfly);

#ifdef __cplusplus
}

void OotmmXflags_Init();
#endif
