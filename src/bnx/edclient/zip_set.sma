/*
 * /opt0/bn/script/edclient/zip_set.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The postal code of an edclient form, seven digits shown as 123-4567: circle starts the edit, left and right
 * choose a digit, up and down change it, circle confirms it ("CHNG" to the manager) and cross gives it up.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

#include "tag.inc"

/* Values of g_iMode. */
enum
{
    MODE_NAVIGATE, /* the focus is on the code */
    MODE_EDIT,     /* the focus is on a digit */
};

/* The arrows of g_objArrows and g_iArrowBlinks. */
const ARROW_UP = 0;   /* above the digit */
const ARROW_DOWN = 1; /* below the digit */

/* Values of g_iArrowBlinks: an arrow blinks when the digit moves away from its end. */
enum
{
    BLINK_NONE,
    BLINK_FADE_IN,
    BLINK_FADE_OUT,
};

#include <float>

new Float:g_flPi = 3.141592;

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
}

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

#include "misc.inc"

#include "background.inc"

public g_objManager; /* the page: gets "CHNG", "PUSH" for cross, "FOCS" when the focus leaves, the code for "GTXT" */
public g_objFocus; /* the focus object, moved onto the code or the digit */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public Float:g_flInterval = 16.0; /* the room of the dash */
public Float:g_flSpeed = 200.0; /* the time of a fade, in ms */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the code has the focus */
new g_bActive; /* the code has the focus and takes the pad */
new g_iMode;
new g_objDigits[7];
new g_objDash;
new g_iCursor; /* the digit edited */
new g_iDigits[7];
new g_iOldDigits[7]; /* the digits before the edit, back on cross */
new g_iArrowBlinks[2];
new g_objArrows[2]; /* above and below the digit */
new g_szText[64]; /* the code, seven digits without the dash; argument "text" first */

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/* Splits the code of argument "text" into its digits and creates them, the dash and the arrows. */
Init()
{
    new i;
    new Float:x;
    new text[64];
    new found;
    SystemCommand(SYSCMD_WIPE_ON_FREE);
    found = GetArg("text", text, 64);
    if (found == 1)
    {
        copy_string(g_szText, text, 64);
    }
    if (strlen(g_szText) == 7)
    {
        i = 0;
        for (; i < 7; i++)
        {
            text{0} = g_szText{i};
            text{1} = 0;
            g_iDigits[i] = atoi(text);
        }
    }
    SetLocalScale(Self(), float(g_iFontSize * 7) + g_flInterval, float(g_iFontSize), 1.0);
    x = -1.0 * (0.5 * (float(g_iFontSize * 5) + g_flInterval + float(g_iFontSize)));
    i = 0;
    for (; i < 7; i++)
    {
        sprintf(text, "%d", g_iDigits[i]);
        g_objDigits[i] = CreateDigitLabel(Self(), x, 0.0, text);
        SetColor(g_objDigits[i], 0.215, 0.708, 0.999, 0.0);
        AnimColor(g_objDigits[i], 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        x = x + float(g_iFontSize);
        if (i == 2)
        {
            x = x + g_flInterval;
        }
    }
    x = -1.0 * (0.5 * (float(g_iFontSize * 5) + g_flInterval + float(g_iFontSize))) + 3 * float(g_iFontSize);
    g_objDash = CreateDigitLabel(Self(), x, 0.0, "-");
    SetColor(g_objDash, 0.7843, 0.7843, 0.7843, 0.0);
    AnimColor(g_objDash, 0.7843, 0.7843, 0.7843, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    {
        new arrow;
        arrow = CreatePlane(Self());
        SetPos(arrow, -2.0, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 0);
        SetScale(arrow, 17.0, 8.0, 1.0);
        SetColor(arrow, 1.0, 1.0, 1.0, 0.0);
        SetRot(arrow, 0.0, 0.0, g_flPi, 0, 0);
        SetTexture(arrow, imgGetResource(RES_SPIN), 0, -1);
        g_objArrows[ARROW_UP] = arrow;
        arrow = CreatePlane(Self());
        SetPos(arrow, -2.0, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 0);
        SetScale(arrow, 17.0, 8.0, 1.0);
        SetColor(arrow, 1.0, 1.0, 1.0, 0.0);
        SetTexture(arrow, imgGetResource(RES_SPIN), 0, -1);
        g_objArrows[ARROW_DOWN] = arrow;
    }
}

/*
 * Handles the messages and the blinks, then the pad while the code has the focus: circle edits it, left and right
 * choose a digit, up and down change it.
 */
Update()
{
    ProcessMessages();
    UpdateArrowBlinks();
    if (0 == g_bActive)
    {
        return;
    }
    new repeated;
    new pressed;
    new text[16];
    PadState(_, pressed, _, repeated, -1);
    switch (g_iMode)
    {
        case MODE_NAVIGATE:
        {
            if (pressed & PAD_CIRCLE)
            {
                g_iMode = MODE_EDIT;
                for (new i; i < 7; i++)
                {
                    g_iOldDigits[i] = g_iDigits[i];
                }
                new Float:x;
                GetPos(g_objDigits[g_iCursor], x, _, _, 0);
                SetPos(g_objArrows[ARROW_UP], -2.0 + x, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 0);
                SetPos(g_objArrows[ARROW_DOWN], -2.0 + x, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 0);
                if (g_iDigits[g_iCursor] < 9)
                {
                    AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                }
                else
                {
                    AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                }
                if (g_iDigits[g_iCursor] > 0)
                {
                    AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                }
                else
                {
                    AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                }
                send_label_focus2(g_objFocus, g_objDigits[g_iCursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                play_se(ED_SE_DECIDE);
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                g_bActive = 0;
            }
            else if (pressed & PAD_RIGHT)
            {
                SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                g_bActive = 0;
            }
            else if (pressed & PAD_LEFT)
            {
                SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                g_bActive = 0;
            }
            else if (pressed & PAD_UP)
            {
                SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                g_bActive = 0;
            }
            else if (pressed & PAD_DOWN)
            {
                SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                g_bActive = 0;
            }
        }
        case MODE_EDIT:
        {
            if (pressed & PAD_CIRCLE)
            {
                g_iMode = MODE_NAVIGATE;
                for (new i; i < 2; i++)
                {
                    g_iArrowBlinks[i] = BLINK_NONE;
                    AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                }
                sprintf(g_szText, "%d%d%d%d%d%d%d", g_iDigits, g_iDigits[1], g_iDigits[2], g_iDigits[3], g_iDigits[4],
                        g_iDigits[5], g_iDigits[6]);
                send_focus(g_objFocus, Self(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                SendMessage(g_objManager, TAG("CHNG"), 0, 0, 0, 0);
                g_bActive = 0;
                play_se(ED_SE_DECIDE);
            }
            else if (pressed & PAD_CROSS)
            {
                g_iMode = MODE_NAVIGATE;
                for (new i; i < 7; i++)
                {
                    g_iDigits[i] = g_iOldDigits[i];
                    sprintf(text, "%d", g_iDigits[i]);
                    SetOptionS(g_objDigits[i], TAG("UTF8"), text);
                }
                for (new i; i < 2; i++)
                {
                    g_iArrowBlinks[i] = BLINK_NONE;
                    AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                }
                send_focus(g_objFocus, Self(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                play_se(ED_SE_CANCEL);
            }
            else if (repeated & PAD_RIGHT)
            {
                if (g_iCursor < 6)
                {
                    g_iCursor++;
                    send_label_focus2(g_objFocus, g_objDigits[g_iCursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    new Float:x;
                    GetPos(g_objDigits[g_iCursor], x, _, _, 0);
                    AnimMoveLine(g_objArrows[ARROW_UP], -2.0 + x, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0,
                            100.0, 0.0, ANIM_FUNC_LINEAR);
                    AnimMoveLine(g_objArrows[ARROW_DOWN], -2.0 + x, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 100.0,
                            0.0, ANIM_FUNC_LINEAR);
                    if (g_iDigits[g_iCursor] < 9)
                    {
                        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    if (g_iDigits[g_iCursor] > 0)
                    {
                        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (repeated & PAD_LEFT)
            {
                if (g_iCursor > 0)
                {
                    g_iCursor--;
                    send_label_focus2(g_objFocus, g_objDigits[g_iCursor], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    new Float:x;
                    GetPos(g_objDigits[g_iCursor], x, _, _, 0);
                    AnimMoveLine(g_objArrows[ARROW_UP], -2.0 + x, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0,
                            100.0, 0.0, ANIM_FUNC_LINEAR);
                    AnimMoveLine(g_objArrows[ARROW_DOWN], -2.0 + x, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 100.0,
                            0.0, ANIM_FUNC_LINEAR);
                    if (g_iDigits[g_iCursor] < 9)
                    {
                        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    if (g_iDigits[g_iCursor] > 0)
                    {
                        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (repeated & PAD_UP)
            {
                if (g_iDigits[g_iCursor] < 9)
                {
                    g_iDigits[g_iCursor]++;
                    sprintf(text, "%d", g_iDigits[g_iCursor]);
                    SetOptionS(g_objDigits[g_iCursor], TAG("UTF8"), text);
                    if (g_iDigits[g_iCursor] == 9)
                    {
                        g_iArrowBlinks[ARROW_UP] = BLINK_NONE;
                        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        BlinkArrow(ARROW_UP);
                    }
                    AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    play_se(ED_SE_CURSOR);
                }
            }
            else if (repeated & PAD_DOWN)
            {
                if (g_iDigits[g_iCursor] > 0)
                {
                    g_iDigits[g_iCursor]--;
                    sprintf(text, "%d", g_iDigits[g_iCursor]);
                    SetOptionS(g_objDigits[g_iCursor], TAG("UTF8"), text);
                    if (0 == g_iDigits[g_iCursor])
                    {
                        g_iArrowBlinks[ARROW_DOWN] = BLINK_NONE;
                        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        BlinkArrow(ARROW_DOWN);
                    }
                    AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
                    play_se(ED_SE_CURSOR);
                }
            }
        }
        default:
        {
        }
    }
}

ProcessMessages()
{
    new msg;
    new a;
    new Float:b;
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the code the pad back. */
    while (RecvMessage(msg, a, b))
    {
        if (msg == TAG("FOCS"))
        {
            HandleFocus(a);
        }
        else if (msg == TAG("FADE"))
        {
            new Float:alpha = a;
            new Float:duration = b;
            for (new i; i < 7; i++)
            {
                AnimColor(g_objDigits[i], 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
            AnimColor(g_objDash, 0.7843, 0.7843, 0.7843, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
        else if (msg == TAG("PUSH"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("CHNG"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("GTXT"))
        {
            SendMessageStr(g_objManager, TAG("GTXT"), g_szText);
        }
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the code. */
HandleFocus(dir)
{
    if (dir == TAG("FOUT"))
    {
        g_bActive = 0;
        return;
    }
    if (dir == TAG("FRST"))
    {
    }
    else if (dir == TAG("RTRN"))
    {
    }
    send_focus(g_objFocus, Self(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    ShowPadHelp();
    g_bActive = 1;
}

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
}

/* Creates the text of a digit, centered at x, y. */
CreateDigitLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
}

/* Starts a blink of arrow index, unless it is blinking. */
BlinkArrow(index)
{
    if (BLINK_NONE != g_iArrowBlinks[index])
    {
        return;
    }
    g_iArrowBlinks[index] = BLINK_FADE_OUT;
    AnimColor(g_objArrows[index], 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
}

/* Fades a blinking arrow back in when it has faded out. */
UpdateArrowBlinks()
{
    for (new i; i < 2; i++)
    {
        if (BLINK_NONE == g_iArrowBlinks[i])
        {
            continue;
        }
        if (ANIM_DONE == AnimGetState(g_objArrows[i], ANIM_COLOR))
        {
            if (g_iArrowBlinks[i] == BLINK_FADE_OUT)
            {
                g_iArrowBlinks[i] = BLINK_FADE_IN;
                AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (g_iArrowBlinks[i] == BLINK_FADE_IN)
            {
                g_iArrowBlinks[i] = BLINK_NONE;
            }
        }
    }
}
