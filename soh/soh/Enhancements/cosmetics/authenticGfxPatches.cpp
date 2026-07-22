#include <libultraship/bridge.h>
#include <string>
#include "soh/OTRGlobals.h"
#include "soh/cvar_prefixes.h"
#include "soh/ResourceManagerHelpers.h"

extern "C" {
#include <libultraship/libultra.h>
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_fz/object_fz.h"
#include "objects/object_gi_soldout/object_gi_soldout.h"
#include "objects/object_ik/object_ik.h"
#include "objects/object_link_child/object_link_child.h"
}

typedef struct {
    const char* dlist;
    int startInstruction;
} DListPatchInfo;

static DListPatchInfo freezardBodyDListPatchInfos[] = {
    { gFreezardIntactDL, 5 },      { gFreezardTopRightHornChippedDL, 5 },
    { gFreezardHeadChippedDL, 5 }, { gFreezardIceTriangleDL, 5 },
    { gFreezardIceRockDL, 5 },
};

static DListPatchInfo ironKnuckleDListPatchInfos[] = {
    // VambraceLeft
    { object_ik_DL_01BE98, 39 },
    { object_ik_DL_01BE98, 59 },

    // ArmLeft
    { object_ik_DL_01C130, 38 },

    // VambraceRight
    { object_ik_DL_01C2B8, 39 },
    { object_ik_DL_01C2B8, 59 },

    // ArmRight
    { object_ik_DL_01C550, 38 },

    // Waist
    { object_ik_DL_01C7B8, 8 },
    { object_ik_DL_01C7B8, 28 },

    // PauldronLeft
    { object_ik_DL_01CB58, 8 },
    { object_ik_DL_01CB58, 31 },

    // BootTipLeft
    { object_ik_DL_01CCA0, 15 },
    { object_ik_DL_01CCA0, 37 },
    { object_ik_DL_01CCA0, 52 },
    { object_ik_DL_01CCA0, 68 },

    // WaistArmorLeft
    { object_ik_DL_01CEE0, 27 },
    { object_ik_DL_01CEE0, 46 },
    { object_ik_DL_01CEE0, 125 },

    // PauldronRight
    { object_ik_DL_01D2B0, 8 },
    { object_ik_DL_01D2B0, 32 },

    // BootTipRight
    { object_ik_DL_01D3F8, 15 },
    { object_ik_DL_01D3F8, 37 },
    { object_ik_DL_01D3F8, 52 },
    { object_ik_DL_01D3F8, 68 },

    // WaistArmorRight
    { object_ik_DL_01D638, 23 },
    { object_ik_DL_01D638, 42 },
    { object_ik_DL_01D638, 110 },
};

static DListPatchInfo arrowTipDListPatchInfos[] = {
    { gArrowNearDL, 46 },
    { gArrowFarDL, 5 },
};

void PatchArrowTipTexture() {
    // Custom texture for Arrow tips that accounts for overflow texture reading
    Gfx arrowTipTextureWithOverflowFixGfx =
        gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b_LOAD_BLOCK, 1, gHilite2Tex_Overflow);

    // Gfx instructions to fix authentic vanilla bug where the Arrow tips texture is read as the wrong size
    Gfx arrowTipTextureWithSizeFixGfx[] = {
        gsDPLoadTextureBlock(gHilite2Tex, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0, G_TX_MIRROR | G_TX_WRAP,
                             G_TX_MIRROR | G_TX_WRAP, 5, 5, 1, 1),
    };

    bool fixTexturesOOB = CVarGetInteger(CVAR_ENHANCEMENT("FixTexturesOOB"), 0);

    for (const auto& patchInfo : arrowTipDListPatchInfos) {
        const char* dlist = patchInfo.dlist;
        int start = patchInfo.startInstruction;

        // Patch using custom overflowed texture
        if (!fixTexturesOOB) {
            // Unpatch the other texture fix
            for (size_t i = 4; i < 8; i++) {
                int instruction = start + i;
                std::string unpatchName = "arrowTipTextureWithSizeFix_" + std::to_string(instruction);
                ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
            }

            std::string patchName = "arrowTipTextureWithOverflowFix_" + std::to_string(start);
            std::string patchName2 = "arrowTipTextureWithOverflowFix_" + std::to_string(start + 1);
            ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), start, arrowTipTextureWithOverflowFixGfx);
            ResourceMgr_PatchGfxByName(dlist, patchName2.c_str(), start + 1, gsSPNoOp());
        } else { // Patch texture to use correct image size/fmt
            // Unpatch the other texture fix
            std::string unpatchName = "arrowTipTextureWithOverflowFix_" + std::to_string(start);
            std::string unpatchName2 = "arrowTipTextureWithOverflowFix_" + std::to_string(start + 1);
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName2.c_str());

            for (size_t i = 4; i < 8; i++) {
                int instruction = start + i;
                std::string patchName = "arrowTipTextureWithSizeFix_" + std::to_string(instruction);

                if (i == 0) {
                    ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction, gsSPNoOp());
                } else {
                    ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction,
                                               arrowTipTextureWithSizeFixGfx[i - 1]);
                }
            }
        }
    }
}

void PatchDekuStickTextureOverflow() {
    // Custom texture for holding Deku Stick that accounts for overflow texture reading
    Gfx dekuSticTexkWithOverflowFixGfx = gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_8b, 1, gDekuStickOverflowTex);

    // Gfx instructions to fix authentic vanilla bug where the Deku Stick texture is read as the wrong size
    Gfx dekuStickTexWithSizeFixGfx[] = {
        gsDPLoadTextureBlock(gDekuStickTex, G_IM_FMT_I, G_IM_SIZ_8b, 8, 8, 0, G_TX_NOMIRROR | G_TX_WRAP,
                             G_TX_NOMIRROR | G_TX_WRAP, 4, 4, G_TX_NOLOD, G_TX_NOLOD),
    };

    const char* dlist = gLinkChildLinkDekuStickDL;
    int start = 5;

    // Patch using custom overflowed texture
    if (!CVarGetInteger(CVAR_ENHANCEMENT("FixTexturesOOB"), 0)) {
        // Unpatch the other texture fix
        for (size_t i = 0; i < 8; i++) {
            int instruction = start + i;
            std::string unpatchName = "dekuStickWithSizeFix_" + std::to_string(instruction);
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
        }

        std::string patchName = "dekuStickWithOverflowFix_" + std::to_string(start);
        std::string patchName2 = "dekuStickWithOverflowFix_" + std::to_string(start + 1);
        ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), start, dekuSticTexkWithOverflowFixGfx);
        ResourceMgr_PatchGfxByName(dlist, patchName2.c_str(), start + 1, gsSPNoOp());
    } else { // Patch texture to use correct image size/fmt
        // Unpatch the other texture fix
        std::string unpatchName = "dekuStickWithOverflowFix_" + std::to_string(start);
        std::string unpatchName2 = "dekuStickWithOverflowFix_" + std::to_string(start + 1);
        ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
        ResourceMgr_UnpatchGfxByName(dlist, unpatchName2.c_str());

        for (size_t i = 0; i < 8; i++) {
            int instruction = start + i;
            std::string patchName = "dekuStickWithSizeFix_" + std::to_string(instruction);

            if (i == 0) {
                ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction, gsSPNoOp());
            } else {
                ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction, dekuStickTexWithSizeFixGfx[i - 1]);
            }
        }
    }
}

void PatchFreezardTextureOverflow() {
    // Custom texture for Freezard effect that accounts for overflow texture reading
    Gfx freezardBodyTextureWithOverflowFixGfx =
        gsDPSetTextureImage(G_IM_FMT_IA, G_IM_SIZ_16b, 1, gEffUnknown12OverflowTex);

    // Gfx instructions to fix authentic vanilla bug where the Freezard effect texture is read as the wrong format
    Gfx freezardBodyTextureWithFormatFixGfx[] = {
        gsDPLoadTextureBlock(gEffUnknown12Tex, G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                             G_TX_NOMIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    };

    bool fixTexturesOOB = CVarGetInteger(CVAR_ENHANCEMENT("FixTexturesOOB"), 0);

    for (const auto& patchInfo : freezardBodyDListPatchInfos) {
        const char* dlist = patchInfo.dlist;
        int start = patchInfo.startInstruction;

        // Patch using custom overflowed texture
        if (!fixTexturesOOB) {
            // Unpatch the other texture fix
            for (size_t i = 0; i < 8; i++) {
                int instruction = start + i;
                std::string unpatchName = "freezardBodyTextureWithFormatFix_" + std::to_string(instruction);
                ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
            }

            std::string patchName = "freezardBodyTextureWithOverflowFix_" + std::to_string(start);
            std::string patchName2 = "freezardBodyTextureWithOverflowFix_" + std::to_string(start + 1);
            ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), start, freezardBodyTextureWithOverflowFixGfx);
            ResourceMgr_PatchGfxByName(dlist, patchName2.c_str(), start + 1, gsSPNoOp());
        } else { // Patch texture to use correct image size/fmt
            // Unpatch the other texture fix
            std::string unpatchName = "freezardBodyTextureWithOverflowFix_" + std::to_string(start);
            std::string unpatchName2 = "freezardBodyTextureWithOverflowFix_" + std::to_string(start + 1);
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName2.c_str());

            for (size_t i = 0; i < 8; i++) {
                int instruction = start + i;
                std::string patchName = "freezardBodyTextureWithFormatFix_" + std::to_string(instruction);

                if (i == 0) {
                    ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction, gsSPNoOp());
                } else {
                    ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction,
                                               freezardBodyTextureWithFormatFixGfx[i - 1]);
                }
            }
        }
    }
}

void PatchIronKnuckleTextureOverflow() {
    // Custom texture for Iron Knuckle that accounts for overflow texture reading
    Gfx ironKnuckleFireTexWithOverflowFixGfx =
        gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_8b, 1, gIronKnuckleMetalOverflowTex);

    // Gfx instructions to fix authentic vanilla bug where the Iron Knuckle texture is read as the wrong format
    Gfx ironKnuckleFireTexWithFormatFixGfx[] = {
        gsDPLoadTextureBlock_4b(gIronKnuckleMetalTex, G_IM_FMT_I, 32, 64, 0, G_TX_MIRROR | G_TX_WRAP,
                                G_TX_MIRROR | G_TX_WRAP, 5, 6, G_TX_NOLOD, G_TX_NOLOD),
    };

    bool fixTexturesOOB = CVarGetInteger(CVAR_ENHANCEMENT("FixTexturesOOB"), 0);

    for (const auto& patchInfo : ironKnuckleDListPatchInfos) {
        const char* dlist = patchInfo.dlist;
        int start = patchInfo.startInstruction;

        // Patch using custom overflowed texture
        if (!fixTexturesOOB) {
            // Unpatch the other texture fix
            for (size_t i = 0; i < 8; i++) {
                int instruction = start + i;
                std::string unpatchName = "ironKnuckleFireTexWithSizeFix_" + std::to_string(instruction);
                ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
            }

            std::string patchName = "ironKnuckleFireTexWithOverflowFix_" + std::to_string(start);
            std::string patchName2 = "ironKnuckleFireTexWithOverflowFix_" + std::to_string(start + 1);
            ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), start, ironKnuckleFireTexWithOverflowFixGfx);
            ResourceMgr_PatchGfxByName(dlist, patchName2.c_str(), start + 1, ironKnuckleFireTexWithOverflowFixGfx);
        } else { // Patch texture to use correct image size/fmt
            // Unpatch the other texture fix
            std::string unpatchName = "ironKnuckleFireTexWithOverflowFix_" + std::to_string(start);
            std::string unpatchName2 = "ironKnuckleFireTexWithOverflowFix_" + std::to_string(start + 1);
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName.c_str());
            ResourceMgr_UnpatchGfxByName(dlist, unpatchName2.c_str());

            for (size_t i = 0; i < 8; i++) {
                int instruction = start + i;
                std::string patchName = "ironKnuckleFireTexWithSizeFix_" + std::to_string(instruction);

                if (i == 0) {
                    ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction, gsSPNoOp());
                } else {
                    ResourceMgr_PatchGfxByName(dlist, patchName.c_str(), instruction,
                                               ironKnuckleFireTexWithFormatFixGfx[i - 1]);
                }
            }
        }
    }
}

void PatchBoulderFragment() {
    // The boulder fragment renders invisible due to the change made by https://github.com/Kenix3/libultraship/pull/721
    // Until it is known whether this change is appropriate or something else should be done to it, the following
    // patches adjust the render mode for the DL to not become invisible
    ResourceMgr_PatchGfxByName(gBoulderFragmentsDL, "boulderFragmentRenderFix3", 3,
                               gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2));
    ResourceMgr_PatchGfxByName(gBoulderFragmentsDL, "boulderFragmentRenderFix6", 6,
                               gsDPSetCombineMode(G_CC_MODULATEIDECALA, G_CC_MODULATEIA_PRIM2));
}

void ApplyAuthenticGfxPatches() {
    // Overflow textures
    PatchArrowTipTexture();
    PatchDekuStickTextureOverflow();
    PatchFreezardTextureOverflow();
    PatchIronKnuckleTextureOverflow();

    PatchBoulderFragment();
}

