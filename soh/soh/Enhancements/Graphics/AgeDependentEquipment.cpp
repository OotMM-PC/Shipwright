#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "soh/OotmmAgeless.h"

extern "C" {
#include "macros.h"
#include "objects/object_link_boy/object_link_boy.h"
#include "objects/object_link_child/object_link_child.h"
extern SaveContext gSaveContext;
}

static constexpr int32_t CVAR_AGE_EQUIPMENT_DEFAULT = 0;
#define CVAR_AGE_EQUIPMENT_NAME CVAR_ENHANCEMENT("EquipmentAlwaysVisible")
#define CVAR_AGE_EQUIPMENT_VALUE CVarGetInteger(CVAR_AGE_EQUIPMENT_NAME, CVAR_AGE_EQUIPMENT_DEFAULT)

static void ResetAdultHands() {
    ResourceMgr_UnpatchGfxByName(gLinkAdultLeftHandHoldingHammerNearDL, "childHammer1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultLeftHandHoldingHammerNearDL, "childHammer2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingHookshotNearDL, "childHookshot1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingHookshotNearDL, "childHookshot2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingBowNearDL, "childBow1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingBowNearDL, "childBow2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultLeftHandHoldingMasterSwordNearDL, "childMasterSword1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultLeftHandHoldingMasterSwordNearDL, "childMasterSword2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultLeftHandHoldingBgsNearDL, "childBiggoronSword1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultLeftHandHoldingBgsNearDL, "childBiggoronSword2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultHandHoldingBrokenGiantsKnifeDL, "childBrokenGiantsKnife1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultHandHoldingBrokenGiantsKnifeDL, "childBrokenGiantsKnife2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingBowFirstPersonDL, "childBowFirstPerson1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingBowFirstPersonDL, "childBowFirstPerson2");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingHookshotFarDL, "childHookshotFirstPerson1");
    ResourceMgr_UnpatchGfxByName(gLinkAdultRightHandHoldingHookshotFarDL, "childHookshotFirstPerson2");
}

static void ResetChildHands() {
    ResourceMgr_UnpatchGfxByName(gLinkChildLeftFistAndKokiriSwordNearDL, "adultKokiriSword");
    ResourceMgr_UnpatchGfxByName(gLinkChildRightHandHoldingSlingshotNearDL, "adultSlingshot");
    ResourceMgr_UnpatchGfxByName(gLinkChildLeftFistAndBoomerangNearDL, "adultBoomerang");
    ResourceMgr_UnpatchGfxByName(gLinkChildRightFistAndDekuShieldNearDL, "adultDekuShield");
    ResourceMgr_UnpatchGfxByName(gLinkChildRightArmStretchedSlingshotDL, "adultSlingshotFirstPerson1");
    ResourceMgr_UnpatchGfxByName(gLinkChildRightArmStretchedSlingshotDL, "adultSlingshotFirstPerson2");
    ResourceMgr_UnpatchGfxByName(gLinkChildRightHandHoldingFairyOcarinaNearDL, "adultFairyOcarina1");
    ResourceMgr_UnpatchGfxByName(gLinkChildRightHandHoldingFairyOcarinaNearDL, "adultFairyOcarina2");
}

static void MakeEquipmentAlwaysVisible() {
    if (LINK_IS_CHILD) {
        ResourceMgr_PatchGfxByName(gLinkAdultLeftHandHoldingHammerNearDL, "childHammer1", 92,
                                   gsSPDisplayListOTRFilePath(gLinkChildLeftFistNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultLeftHandHoldingHammerNearDL, "childHammer2", 93, gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingHookshotNearDL, "childHookshot1", 84,
                                   gsSPDisplayListOTRFilePath(gLinkChildRightHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingHookshotNearDL, "childHookshot2", 85,
                                   gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingBowNearDL, "childBow1", 51,
                                   gsSPDisplayListOTRFilePath(gLinkChildRightHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingBowNearDL, "childBow2", 52, gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultLeftHandHoldingMasterSwordNearDL, "childMasterSword1", 104,
                                   gsSPDisplayListOTRFilePath(gLinkChildLeftFistNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultLeftHandHoldingMasterSwordNearDL, "childMasterSword2", 105,
                                   gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultLeftHandHoldingBgsNearDL, "childBiggoronSword1", 79,
                                   gsSPDisplayListOTRFilePath(gLinkChildLeftFistNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultLeftHandHoldingBgsNearDL, "childBiggoronSword2", 80, gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultHandHoldingBrokenGiantsKnifeDL, "childBrokenGiantsKnife1", 76,
                                   gsSPDisplayListOTRFilePath(gLinkChildLeftFistNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultHandHoldingBrokenGiantsKnifeDL, "childBrokenGiantsKnife2", 77,
                                   gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingBowFirstPersonDL, "childBowFirstPerson1", 58,
                                   gsSPDisplayListOTRFilePath(gLinkChildRightHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingBowFirstPersonDL, "childBowFirstPerson2", 59,
                                   gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingHookshotFarDL, "childHookshotFirstPerson1", 114,
                                   gsSPDisplayListOTRFilePath(gLinkChildRightHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkAdultRightHandHoldingHookshotFarDL, "childHookshotFirstPerson2", 115,
                                   gsSPEndDisplayList());
        ResetChildHands();
    } else {
        ResourceMgr_PatchGfxByName(gLinkChildLeftFistAndKokiriSwordNearDL, "adultKokiriSword", 13,
                                   gsSPDisplayListOTRFilePath(gLinkAdultLeftHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkChildRightHandHoldingSlingshotNearDL, "adultSlingshot", 13,
                                   gsSPDisplayListOTRFilePath(gLinkAdultRightHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkChildLeftFistAndBoomerangNearDL, "adultBoomerang", 50,
                                   gsSPDisplayListOTRFilePath(gLinkAdultLeftHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkChildRightFistAndDekuShieldNearDL, "adultDekuShield", 49,
                                   gsSPDisplayListOTRFilePath(gLinkAdultRightHandClosedNearDL));
        ResourceMgr_PatchGfxByName(gLinkChildRightArmStretchedSlingshotDL, "adultSlingshotFirstPerson1", 30,
                                   gsSPDisplayListOTRFilePath(gLinkAdultRightHandNearDL));
        ResourceMgr_PatchGfxByName(gLinkChildRightArmStretchedSlingshotDL, "adultSlingshotFirstPerson2", 31,
                                   gsSPEndDisplayList());
        ResourceMgr_PatchGfxByName(gLinkChildRightHandHoldingFairyOcarinaNearDL, "adultFairyOcarina1", 37,
                                   gsSPDisplayListOTRFilePath(gLinkAdultRightHandNearDL));
        ResourceMgr_PatchGfxByName(gLinkChildRightHandHoldingFairyOcarinaNearDL, "adultFairyOcarina2", 38,
                                   gsSPEndDisplayList());
        ResetAdultHands();
    }
}

static void RegisterAgeDependentEquipmentHook() {
    if (CVAR_AGE_EQUIPMENT_VALUE) {
        MakeEquipmentAlwaysVisible();
    } else {
        ResetAdultHands();
        ResetChildHands();
    }

    COND_HOOK(OnSceneInit, true, [](int32_t) {
        if (OotmmAgeless_EquipmentVisible()) {
            MakeEquipmentAlwaysVisible();
        } else {
            ResetAdultHands();
            ResetChildHands();
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterAgeDependentEquipmentHook, { CVAR_AGE_EQUIPMENT_NAME });
