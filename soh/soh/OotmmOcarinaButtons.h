#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OOTMM_OCARINA_BUTTON_A = 0,
    OOTMM_OCARINA_BUTTON_C_DOWN,
    OOTMM_OCARINA_BUTTON_C_RIGHT,
    OOTMM_OCARINA_BUTTON_C_LEFT,
    OOTMM_OCARINA_BUTTON_C_UP,
} OotmmOcarinaButton;

int32_t Ootmm_IsOcarinaButtonAvailable(int32_t button);
void Ootmm_GateOcarinaButtonMaps(int32_t* d4, int32_t* d5, int32_t* b4, int32_t* a4, int32_t* f4);

#ifdef __cplusplus
}
#endif
