#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

int OotmmSilverRupeesSolved(struct PlayState* play, int room);

int OotmmSilverRupeesPuzzleIds(int scene, int room, int* base, int* width);

void OotmmSilverRupeesSyncFlags(struct PlayState* play);
void OotmmSilverRupeesUpdateTempFlags(void);

int OotmmSilverRupeesSceneBase(int scene);

int OotmmSilverRupeesRequired(const char* suffix);

#ifdef __cplusplus
}
#endif
