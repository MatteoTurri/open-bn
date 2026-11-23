/*
 * /opt0/bn/script/edclient/spin.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A number of an edclient page, from g_iMin to g_iMax: circle starts the edit, up and down change it between two
 * arrows, circle confirms it ("CHNG" to the manager) and cross gives it up.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "edpage.inc"

#include "tag.inc"

/* Values of g_iMode. */
enum
{
    MODE_NAVIGATE, /* the focus is on the number */
    MODE_EDIT,     /* up and down change the number */
};

/* The arrows of g_objArrows and g_iArrowBlinks. */
const ARROW_UP = 0;   /* above the number */
const ARROW_DOWN = 1; /* below the number */

/* Values of g_iArrowBlinks: an arrow blinks when the number moves away from its end. */
enum
{
    BLINK_NONE,
    BLINK_FADE_IN,
    BLINK_FADE_OUT,
};

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

#include "misc.inc"

#include "background.inc"

public g_objManager; /* the page: gets "PUSH", "CHNG", "FOCS" when the focus leaves, and the text for "GTXT" */
public g_objFocus; /* the focus object, moved onto the number */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public Float:g_flSpeed = 200.0; /* the time of a fade, in ms */
public g_iPadHelp; /* the SYS_PADHELP buttons while the number has the focus, besides circle */
public g_iFigure; /* the digits of the number, padded with zeros; 0: as many as needed */
public g_iMin; /* the range of the number */
public g_iMax; /* the range of the number */
new g_bActive; /* the number has the focus and takes the pad */
new g_iMode;
new g_szText[64]; /* the number as shown; argument "text" first */
new g_objLabel;
new g_iValue;
new g_iOldValue; /* the number before the edit, back on cross */
new g_iArrowBlinks[2];
new g_objArrows[2]; /* above and below the number */
new g_iRepeated;
new g_iPressed;

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/* Reads the number of argument "text", g_iMin without it, and creates its text and its two arrows. */
Init()
{
    new text[64];
    new found;
    found = GetArg("text", text, 64);
    if (found == 1)
    {
        copy_string(g_szText, text, 64);
    }
    if (strlen(g_szText) > 0)
    {
        g_iValue = atoi(g_szText);
    }
    else
    {
        g_iValue = g_iMin;
        FormatValue();
    }
    g_objLabel = CreateValueLabel(Self(), 0.0, 0.0, g_szText);
    AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    g_objArrows[ARROW_UP] = CreatePlane(Self());
    SetPos(g_objArrows[ARROW_UP], 0.0, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 0);
    SetScale(g_objArrows[ARROW_UP], 17.0, 8.0, 1.0);
    SetColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0);
    SetRot(g_objArrows[ARROW_UP], 0.0, 0.0, g_flPi, 0, 0);
    SetTexture(g_objArrows[ARROW_UP], imgGetResource(RES_SPIN), 0, -1);
    g_objArrows[ARROW_DOWN] = CreatePlane(Self());
    SetPos(g_objArrows[ARROW_DOWN], 0.0, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 0);
    SetScale(g_objArrows[ARROW_DOWN], 17.0, 8.0, 1.0);
    SetColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objArrows[ARROW_DOWN], imgGetResource(RES_SPIN), 0, -1);
}

/* Handles the messages and the blinks, then the pad while the number has the focus, by g_iMode. */
Update()
{
    ProcessMessages();
    UpdateArrowBlinks();
    if (0 == g_bActive)
    {
        return;
    }
    PadState(_, g_iPressed, _, g_iRepeated, -1);
    switch (g_iMode)
    {
        case MODE_NAVIGATE:
        {
            UpdateNavigate();
        }
        case MODE_EDIT:
        {
            UpdateEdit();
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
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the number the pad back. */
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
            AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
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

/* "FOUT" takes the focus away; any other dir gives it to the number. */
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
    send_label_focus2(g_objFocus, g_objLabel, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    ShowPadHelp();
    g_bActive = 1;
}

/*
 * Circle tells the manager "PUSH" and starts the edit, which takes the pad when the manager gives it back; cross and
 * the directions go to the manager.
 */
UpdateNavigate()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        g_bActive = 0;
        g_iMode = MODE_EDIT;
        g_iOldValue = g_iValue;
        if (g_iValue != g_iMax)
        {
            AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        if (g_iValue != g_iMin)
        {
            AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        play_se(ED_SE_DECIDE);
    }
    else if (g_iPressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_RIGHT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_LEFT)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_UP)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (g_iPressed & PAD_DOWN)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
        g_bActive = 0;
    }
}

/* Up and down change the number within g_iMin and g_iMax; circle confirms it, cross restores it. */
UpdateEdit()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_NAVIGATE;
        for (new i; i < 2; i++)
        {
            g_iArrowBlinks[i] = BLINK_NONE;
            AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        SendMessage(g_objManager, TAG("CHNG"), 0, 0, 0, 0);
        g_bActive = 0;
        play_se(ED_SE_DECIDE);
    }
    else if (g_iPressed & PAD_CROSS)
    {
        g_iMode = MODE_NAVIGATE;
        g_iValue = g_iOldValue;
        FormatValue();
        SetOptionS(g_objLabel, TAG("UTF8"), g_szText);
        for (new i; i < 2; i++)
        {
            g_iArrowBlinks[i] = BLINK_NONE;
            AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        play_se(ED_SE_CANCEL);
    }
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iValue < g_iMax)
        {
            g_iValue++;
            FormatValue();
            SetOptionS(g_objLabel, TAG("UTF8"), g_szText);
            if (g_iValue == g_iMax)
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
    else if (g_iRepeated & PAD_DOWN)
    {
        if (g_iValue > g_iMin)
        {
            g_iValue--;
            FormatValue();
            SetOptionS(g_objLabel, TAG("UTF8"), g_szText);
            if (g_iValue == g_iMin)
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

/* Creates the text of the number, centered at x, y, hidden. */
CreateValueLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
}

/* Writes g_iValue into g_szText with g_iFigure digits. */
FormatValue()
{
    new format[64];
    if (0 == g_iFigure)
    {
        copy_string(format, "%d", 64);
    }
    else
    {
        sprintf(format, "%%0%dd", g_iFigure);
    }
    sprintf(g_szText, format, g_iValue);
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

/* Shows the buttons of g_iPadHelp and circle in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp | PADHELP_CIRCLE);
}
