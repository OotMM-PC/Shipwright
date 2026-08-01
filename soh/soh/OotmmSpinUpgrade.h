#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Highest spin attack charge Link may reach, halved until the seed's Spin Attack Upgrade is found.
float OotmmSpinUpgrade_ChargeLimit(void);

/// 1 forces the wide spin, 0 the narrow one, -1 leaves the choice to Link's charge.
int32_t OotmmSpinUpgrade_SpinLevel(void);

#ifdef __cplusplus
}
#endif
