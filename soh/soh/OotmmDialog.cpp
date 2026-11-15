#include "Enhancements/custom-message/CustomMessageManager.h"
#include "OotmmDialog.h"
#include "cvar_prefixes.h"

#include <libultraship/bridge/OotmmDialog.h>
#include <libultraship/libultraship.h>

#include <algorithm>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern const char* fontTbl[140];
extern f32 sFontWidths[144];
extern PlayState* gPlayState;
void Message_DrawTextChar(PlayState* play, void* textureImage, Gfx** p);
void Message_SetTextColor(MessageContext* msgCtx, u16 colorParameter);
}

namespace {

constexpr u16 kStandaloneTextId = 0x88C;
constexpr s16 kBoxWidth = 256;
constexpr s16 kLineHeight = 12;
constexpr s16 kTextTop = 8;
constexpr s16 kTextRightInset = 16;
constexpr s16 kOptionInset = 10;
constexpr s16 kBarRightInset = 10;
constexpr s16 kBarWidth = 3;
constexpr s16 kHiddenY = 300;

bool sActive = false;
bool sBuilt = false;
Ship::OotmmDialogSpec sSpec;
Ship::OotmmDialogLayout sLayout;
Ship::OotmmDialogController sController;
std::function<void(int)> sOnChoice;
u32 sHeldFrame = 0;
bool sHolding = false;
s16 sLeft = 0;
bool sHoldingPlayer = false;

void HoldPlayer(PlayState* play, bool hold) {
    if (play != nullptr && hold != sHoldingPlayer) {
        Player_SetCsActionWithHaltedActors(play, nullptr, hold ? PLAYER_CSACTION_8 : PLAYER_CSACTION_7);
    }
    sHoldingPlayer = hold;
}

void Assign(std::string body, std::vector<std::string> options) {
    options.resize(std::clamp<size_t>(options.size(), 2, 3));
    sSpec = {};
    sSpec.Body = std::move(body);
    sSpec.Options = std::move(options);
}

void LoadEngineText() {
    CustomMessage message(sSpec.Options.size() == 3 ? "\x1C" : "\x1B");
    message.Format();
    message.LoadIntoFont();
}

void Activate() {
    sActive = true;
    sBuilt = false;
    sHolding = false;
}

bool IsChoosing(PlayState* play) {
    const MessageContext* msgCtx = &play->msgCtx;
    return sActive && msgCtx->msgLength != 0 && msgCtx->msgMode == MSGMODE_TEXT_DONE &&
           (msgCtx->textboxEndType == TEXTBOX_ENDTYPE_2_CHOICE || msgCtx->textboxEndType == TEXTBOX_ENDTYPE_3_CHOICE);
}

void Build() {
    if (sBuilt) {
        return;
    }
    Ship::OotmmDialogFont font;
    for (size_t i = 0; i < font.Advance.size(); ++i) {
        font.Advance[i] = static_cast<int16_t>(static_cast<s32>(sFontWidths[i] * (R_TEXT_CHAR_SCALE / 100.0f)));
    }
    font.Advance[0] = static_cast<int16_t>(CVarGetInteger(CVAR_ENHANCEMENT("TextSpacing"), 6));

    sLeft = R_TEXT_INIT_XPOS;
    const s16 right = R_TEXTBOX_X_TARGET + kBoxWidth - kTextRightInset;
    Ship::OotmmDialogBox box;
    box.BodyWidth = right - sLeft;
    box.OptionWidth = right - sLeft - kOptionInset;
    sLayout = Ship::OotmmDialogLayOut(sSpec, font, box);
    sController = Ship::OotmmDialogController(sLayout);
    sBuilt = true;
}

s16 RowY(int row) {
    const s16 centering = static_cast<s16>((sLayout.VisibleLines - sController.View().ShownLines()) * kLineHeight / 2);
    return static_cast<s16>(R_TEXTBOX_Y + kTextTop + centering + row * kLineHeight);
}

u16 ColorOf(Ship::OotmmDialogColor color) {
    static constexpr u16 kDefault = 0, kRed = 1, kGreen = 2, kBlue = 3, kLightBlue = 4, kPink = 5, kYellow = 6;
    static constexpr u16 kTable[] = { kDefault, kRed, kGreen, kBlue, kLightBlue, kPink, kYellow, kYellow };
    return kTable[static_cast<size_t>(color)];
}

void DrawBar(PlayState* play, Gfx** gfxP) {
    if (!sController.View().Scrollable()) {
        return;
    }
    const MessageContext* msgCtx = &play->msgCtx;
    const s16 track = sLayout.VisibleLines * kLineHeight;
    const auto thumb = sController.View().Thumb(track);
    const s16 x = R_TEXTBOX_X_TARGET + kBoxWidth - kBarRightInset;
    const s16 y = R_TEXTBOX_Y + kTextTop;
    Gfx* gfx = *gfxP;
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, msgCtx->textColorAlpha / 4);
    gDPFillRectangle(gfx++, x, y, x + kBarWidth, y + track);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, msgCtx->textColorAlpha);
    gDPFillRectangle(gfx++, x, y + thumb.Offset, x + kBarWidth, y + thumb.Offset + thumb.Length);
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    *gfxP = gfx;
}

} // namespace

namespace OotmmDialog {

void Load(std::string body, std::vector<std::string> options) {
    Assign(std::move(body), std::move(options));
    LoadEngineText();
    sOnChoice = nullptr;
    Activate();
}

void Start(PlayState* play, std::string body, std::vector<std::string> options, std::function<void(int)> onChoice) {
    Assign(std::move(body), std::move(options));
    Message_StartTextbox(play, kStandaloneTextId, nullptr);
    LoadEngineText();
    sOnChoice = std::move(onChoice);
    Activate();
    HoldPlayer(play, true);
}

DebugState Debug() {
    DebugState state;
    if (gPlayState == nullptr || !sActive || !sBuilt || gPlayState->msgCtx.msgLength == 0) {
        return state;
    }
    state.Active = true;
    state.Top = sController.View().Top();
    state.Total = static_cast<int>(sLayout.Lines.size());
    state.Choice = gPlayState->msgCtx.choiceIndex;
    state.Scrollable = sController.View().Scrollable();
    for (int row = 0; row < sController.View().ShownLines(); ++row) {
        std::string text;
        for (const auto& c : sLayout.Lines[static_cast<size_t>(sController.View().Top() + row)].Chars) {
            text.push_back(c.Code);
        }
        state.Shown.push_back(text);
    }
    for (const auto first : sLayout.OptionFirstLine) {
        state.OptionRows.push_back(sController.View().Row(first));
        state.OptionLines.push_back(first);
    }
    if (state.Choice >= 0 && state.Choice < static_cast<int>(state.OptionRows.size())) {
        state.CursorRow = state.OptionRows[static_cast<size_t>(state.Choice)];
    }
    return state;
}

} // namespace OotmmDialog

extern "C" void OotmmDialog_Reset(void) {
    sActive = false;
    sOnChoice = nullptr;
    HoldPlayer(gPlayState, false);
}

extern "C" void OotmmDialog_Draw(PlayState* play, Gfx** gfxP) {
    if (!sActive) {
        return;
    }
    Build();
    MessageContext* msgCtx = &play->msgCtx;
    const s16 savedX = msgCtx->textPosX;
    const s16 savedY = msgCtx->textPosY;
    const s16 savedR = msgCtx->textColorR;
    const s16 savedG = msgCtx->textColorG;
    const s16 savedB = msgCtx->textColorB;

    for (int row = 0; row < sController.View().ShownLines(); ++row) {
        const auto& line = sLayout.Lines[static_cast<size_t>(sController.View().Top() + row)];
        const s16 x = sLeft + (line.Option >= 0 ? kOptionInset : 0);
        msgCtx->textPosY = RowY(row);
        for (const auto& c : line.Chars) {
            if (c.Code == ' ') {
                continue;
            }
            Message_SetTextColor(msgCtx, ColorOf(c.Color));
            msgCtx->textPosX = x + c.X;
            Message_DrawTextChar(play, (void*)fontTbl[c.Code - ' '], gfxP);
        }
    }
    DrawBar(play, gfxP);

    msgCtx->textPosX = savedX;
    msgCtx->textPosY = savedY;
    msgCtx->textColorR = savedR;
    msgCtx->textColorG = savedG;
    msgCtx->textColorB = savedB;
}

extern "C" s16 OotmmDialog_CursorY(PlayState* play, s16 vanillaY) {
    if (!sActive) {
        return vanillaY;
    }
    Build();
    const size_t choice = play->msgCtx.choiceIndex;
    if (choice >= sLayout.OptionFirstLine.size()) {
        return kHiddenY;
    }
    const int row = sController.View().Row(sLayout.OptionFirstLine[choice]);
    return row < 0 ? kHiddenY : RowY(row);
}

extern "C" void OotmmDialog_HandleInput(PlayState* play) {
    if (!IsChoosing(play)) {
        return;
    }
    Build();
    MessageContext* msgCtx = &play->msgCtx;
    const Input* input = &play->state.input[0];
    const bool dpadFree = CVarGetInteger(CVAR_SETTING("DpadInText"), 0) == 0;
    Ship::OotmmDialogInput navigation;
    navigation.Choice = msgCtx->choiceIndex;
    navigation.StickY = input->rel.stick_y;
    navigation.ScrollStickY = input->cur.right_stick_y;
    navigation.ScrollUp = dpadFree && CHECK_BTN_ALL(input->cur.button, BTN_DUP);
    navigation.ScrollDown = dpadFree && CHECK_BTN_ALL(input->cur.button, BTN_DDOWN);
    const auto outcome = sController.Navigate(navigation);
    msgCtx->choiceIndex = static_cast<u8>(outcome.Choice);
    if (outcome.Scrolled) {
        Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    }
}

extern "C" void OotmmDialog_Confirm(PlayState* play) {
    if (!sActive || !sOnChoice) {
        return;
    }
    const auto onChoice = std::move(sOnChoice);
    sOnChoice = nullptr;
    HoldPlayer(play, false);
    onChoice(play->msgCtx.choiceIndex);
}

extern "C" s32 OotmmDialog_HoldAdvance(PlayState* play) {
    if (!IsChoosing(play)) {
        return false;
    }
    Build();
    if (sHolding && sHeldFrame == play->state.frames) {
        return true;
    }
    sHolding = false;
    const Input* input = &play->state.input[0];
    const bool confirms = CHECK_BTN_ANY(input->press.button, BTN_A | BTN_B);
    const bool stickOnly = !confirms && CHECK_BTN_ALL(input->press.button, BTN_CUP) && sController.View().Scrollable();
    if (stickOnly) {
        sHolding = true;
        sHeldFrame = play->state.frames;
        return true;
    }
    if ((confirms || CHECK_BTN_ALL(input->press.button, BTN_CUP)) && sController.GuardConfirm()) {
        sHolding = true;
        sHeldFrame = play->state.frames;
        return true;
    }
    return false;
}
