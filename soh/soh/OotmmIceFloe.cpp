extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
}

#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"

#include "OotmmIceFloe.h"
#include "OotmmSession.h"

#include <cmath>

extern "C" int16_t gOotmmIceFloeId = -1;

namespace {

constexpr const char* kIceFloeCol = "__OTR__objects/mm_obj_icefloe/gIcefloePlatformCol";
constexpr const char* kIceFloeDL = "__OTR__objects/mm_obj_icefloe/gIcefloeIcePlatformDL";
constexpr int kMaxFloes = 3;

OotmmIceFloe* sSpawnedInstances[kMaxFloes] = { nullptr, nullptr, nullptr };
int32_t sNumberSpawned = 0;

void Grow(OotmmIceFloe* self, PlayState* play);
void Float(OotmmIceFloe* self, PlayState* play);
void Melt(OotmmIceFloe* self, PlayState* play);

void StartGrow(OotmmIceFloe* self, PlayState* play) {
    self->timer = 20;
    SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &self->dyna.actor.world.pos,
                                 &self->dyna.actor.projectedPos, &self->dyna.actor.projectedW);
    Audio_PlayActorSound2(&self->dyna.actor, NA_SE_EV_ICE_MELT);
    self->actionFunc = Grow;
}

void StartFloat(OotmmIceFloe* self) {
    self->timer = self->dyna.actor.params;
    self->actionFunc = Float;
}

void StartMelt(OotmmIceFloe* self) {
    self->timer = 50;
    self->dyna.actor.scale.y = 0.13f;
    Audio_PlayActorSound2(&self->dyna.actor, NA_SE_EV_ICE_MELT);
    self->actionFunc = Melt;
}

void Grow(OotmmIceFloe* self, PlayState* play) {
    self->dyna.actor.scale.x += 0.65f * 0.01f;
    self->dyna.actor.scale.z += 0.65f * 0.01f;
    self->dyna.actor.scale.y += 0.65f * 0.01f;
    self->timer--;
    if (self->timer == 0) {
        StartFloat(self);
    }
}

void Float(OotmmIceFloe* self, PlayState* play) {
    WaterBox* waterBox;

    self->timer--;
    if ((self->timer == 0) ||
        !WaterBox_GetSurface1(play, &play->colCtx, self->dyna.actor.world.pos.x, self->dyna.actor.world.pos.z,
                              &self->dyna.actor.home.pos.y, &waterBox)) {
        StartMelt(self);
    } else {
        self->dyna.actor.scale.y = LINK_IS_ADULT ? 0.20f : 0.13f;
        self->dyna.actor.world.pos.y =
            (Math_SinF(self->timer * (static_cast<float>(M_PI) / 30.0f)) * 3.0f) + (self->dyna.actor.home.pos.y + 10.0f);
    }
}

void Melt(OotmmIceFloe* self, PlayState* play) {
    self->timer--;
    if ((self->timer >= 38) && !(self->timer % 2)) {
        Vec3f velocity;
        Vec3f position;

        velocity.y = (self->timer - 38) * (1 / 12.0f);
        velocity.x = Rand_CenteredFloat(1.5f) * velocity.y;
        velocity.z = Rand_CenteredFloat(1.5f) * velocity.y;
        velocity.y += 0.8f;
        position.x = self->dyna.actor.world.pos.x + (2.0f * velocity.x);
        position.z = self->dyna.actor.world.pos.z + (2.0f * velocity.z);
        position.y = self->dyna.actor.world.pos.y + 3.0f;
        Vec3f accel = { 0.0f, 0.0f, 0.0f };
        EffectSsIceSmoke_Spawn(play, &position, &velocity, &accel, 200);
    }
    if (self->timer < 25) {
        self->dyna.actor.scale.x -= 0.0052f;
        self->dyna.actor.scale.z -= 0.0052f;
    }
    self->dyna.actor.scale.y -= 0.0026f;
    if (self->dyna.actor.scale.y <= 0.0f) {
        Actor_Kill(&self->dyna.actor);
    }
}

} // namespace

extern "C" void OotmmIceFloe_Init(Actor* thisx, PlayState* play) {
    OotmmIceFloe* self = reinterpret_cast<OotmmIceFloe*>(thisx);
    CollisionHeader* colHeader = nullptr;

    Actor_SetScale(&self->dyna.actor, 0.0f);
    DynaPolyActor_Init(&self->dyna, 0);
    CollisionHeader_GetVirtual(const_cast<char*>(kIceFloeCol), &colHeader);
    self->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &self->dyna.actor, colHeader);
    if (sNumberSpawned >= kMaxFloes) {
        if ((sSpawnedInstances[0] != nullptr) && (sSpawnedInstances[0]->dyna.actor.update != nullptr) &&
            (sSpawnedInstances[0]->actionFunc != Melt)) {
            StartMelt(sSpawnedInstances[0]);
        }
        for (int i = 0; i < kMaxFloes - 1; i++) {
            sSpawnedInstances[i] = sSpawnedInstances[i + 1];
        }
        sSpawnedInstances[kMaxFloes - 1] = self;
    } else {
        sSpawnedInstances[sNumberSpawned] = self;
    }
    sNumberSpawned++;
    self->dyna.actor.room = -1;
    self->dyna.actor.world.pos.y = self->dyna.actor.home.pos.y + 10.0f;
    StartGrow(self, play);
}

extern "C" void OotmmIceFloe_Destroy(Actor* thisx, PlayState* play) {
    OotmmIceFloe* self = reinterpret_cast<OotmmIceFloe*>(thisx);

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, self->dyna.bgId);
    sNumberSpawned--;
    for (auto& instance : sSpawnedInstances) {
        if (instance == self) {
            instance = nullptr;
            break;
        }
    }
}

extern "C" void OotmmIceFloe_Update(Actor* thisx, PlayState* play) {
    OotmmIceFloe* self = reinterpret_cast<OotmmIceFloe*>(thisx);

    if (!Play_InCsMode(play)) {
        self->actionFunc(self, play);
    }
}

extern "C" void OotmmIceFloe_Draw(Actor* thisx, PlayState* play) {
    Gfx_DrawDListOpa(play, (Gfx*)kIceFloeDL);
}

extern "C" void OotmmIceFloe_CheckArrow(Actor* arrow, PlayState* play) {
    if (gOotmmIceFloeId < 0 || !OotmmSession_IsActive() || arrow->params != ARROW_ICE ||
        !OotmmSession_GetState().GetBoolSetting("iceArrowPlatformsOot", false)) {
        return;
    }
    f32 height = arrow->world.pos.y;
    WaterBox* waterBox;
    if (!WaterBox_GetSurface1(play, &play->colCtx, arrow->world.pos.x, arrow->world.pos.z, &height, &waterBox) ||
        arrow->world.pos.y >= height) {
        return;
    }
    Vec3f delta;
    Math_Vec3f_Diff(&arrow->world.pos, &arrow->prevPos, &delta);
    if (delta.y != 0.0f) {
        f32 horizontal = sqrtf(SQ(delta.x) + SQ(delta.z));
        if (horizontal != 0.0f) {
            horizontal = (((height - arrow->prevPos.y) / delta.y) * horizontal) / horizontal;
        }
        delta.x = arrow->prevPos.x + (delta.x * horizontal);
        delta.y = height;
        delta.z = arrow->prevPos.z + (delta.z * horizontal);
    }
    Actor_Spawn(&play->actorCtx, play, gOotmmIceFloeId, delta.x, delta.y, delta.z, 0, 0, 0, 300);
    Actor_Kill(arrow);
}
