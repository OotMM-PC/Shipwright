#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

/// Solved from the launcher inventory alone, without collecting anything in the room.
int OotmmSilverRupeesSolved(struct PlayState* play, int room);

/// Writes the save-bit id range OoTMM assigns this room, or returns 0 leaving the outputs alone.
int OotmmSilverRupeesPuzzleIds(int scene, int room, int* base, int* width);

/// Lowest puzzle id in this scene, or -1 when the scene holds no shuffled puzzle.
int OotmmSilverRupeesSceneBase(int scene);

/// Counted against the quest the puzzle's own scene is running; 0 when it has none there.
int OotmmSilverRupeesRequired(const char* suffix);

#ifdef __cplusplus
}
#endif
