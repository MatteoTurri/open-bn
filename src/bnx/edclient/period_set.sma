/*
 * /opt0/bn/script/edclient/period_set.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A month and a year of an edclient form ("YYYYMM"), within the "min" and "max" of the "param" children of its
 * parts element: circle starts the edit, left and right choose the field, up and down change it.
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
    MODE_NAVIGATE, /* the focus is on the date */
    MODE_EDIT,     /* the focus is on the month or the year */
};

/* The fields of the date: the values of g_iField, the index of the arrays of the fields. */
enum
{
    FIELD_MONTH,
    FIELD_YEAR,
};

/* The arrows of g_objArrows and g_iArrowBlinks. */
const ARROW_UP = 0;   /* above the field */
const ARROW_DOWN = 1; /* below the field */

/* Values of g_iArrowBlinks: an arrow blinks when the field moves away from its end. */
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

#include "text.inc"

/* Same as send_focus of dom.inc. */
send_focus2(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
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

#include "dom.inc"

#include "background.inc"

public g_objManager; /* the page: gets "CHNG", "PUSH" for cross, "FOCS" when the focus leaves, the date for "GTXT" */
public g_objFocus; /* the focus object, moved onto the date or the field */
public g_tagParts; /* the element of the date: "w", "h", and a "param" child for the month and one for the year */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the date has the focus */
new g_bActive; /* the date has the focus and takes the pad */
new g_iMode;
new g_iField; /* the field edited (FIELD_*) */
new Float:g_flSpeed = 200.0; /* the time of a fade, in ms */
new g_iFontSize = 15;
new g_objPeriod; /* an empty object as large as the date, for the focus */
new g_objFields[2];
new g_iValues[2];
new g_iOldValues[2]; /* the values before the edit, back on cross */
new g_iMins[2];
new g_iMaxs[2];
new g_iDigits[2]; /* the digits of a field, padded with zeros */
new g_szText[64]; /* the date, "YYYYMM"; argument "text" first */
new g_szFields[2][16];
new g_iArrowBlinks[2];
new g_objArrows[2]; /* above and below the field */
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

/*
 * Splits the date of argument "text" into its fields, reads their place and range from the "param" children and creates
 * them.
 */
Init()
{
    new value[64];
    new found;
    SystemCommand(SYSCMD_WIPE_ON_FREE);
    found = GetArg("text", value, 64);
    if (found == 1)
    {
        copy_string(g_szText, value, 64);
    }
    copy_substring(g_szFields[FIELD_YEAR], 0, g_szText, 0, 4, 16);
    copy_substring(g_szFields[FIELD_MONTH], 0, g_szText, 4, 2, 16);
    new Float:x;
    new Float:y;
    new Float:box_w;
    new Float:box_h;
    new param;
    x = 0.0;
    y = 0.0;
    box_w = float(g_iFontSize);
    box_h = float(g_iFontSize);
    found = domGetData(g_tagParts, "w", value, 64);
    if (found == 1)
    {
        box_w = atof(value);
    }
    found = domGetData(g_tagParts, "h", value, 64);
    if (found == 1)
    {
        box_h = atof(value);
    }
    x = x + 0.5 * box_w;
    g_objPeriod = CreateNull(Self());
    SetPos(g_objPeriod, x, y, 0.0, 0);
    SetLocalScale(g_objPeriod, box_w, box_h, 1.0);
    param = find_child_tag(g_tagParts, "param", "type", "month");
    x = 0.0;
    y = 0.0;
    found = domGetData(param, "x", value, 64);
    if (found == 1)
    {
        x = atof(value);
    }
    found = domGetData(param, "y", value, 64);
    if (found == 1)
    {
        y = atof(value);
    }
    found = domGetData(param, "min", value, 64);
    if (found == 1)
    {
        g_iMins[FIELD_MONTH] = atoi(value);
    }
    found = domGetData(param, "max", value, 64);
    if (found == 1)
    {
        g_iMaxs[FIELD_MONTH] = atoi(value);
    }
    g_objFields[FIELD_MONTH] = CreateFieldLabel(Self(), x, y, g_szFields[FIELD_MONTH]);
    AnimColor(g_objFields[FIELD_MONTH], 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    param = find_child_tag(g_tagParts, "param", "type", "year");
    x = 0.0;
    y = 0.0;
    found = domGetData(param, "x", value, 64);
    if (found == 1)
    {
        x = atof(value);
    }
    found = domGetData(param, "y", value, 64);
    if (found == 1)
    {
        y = atof(value);
    }
    found = domGetData(param, "min", value, 64);
    if (found == 1)
    {
        g_iMins[FIELD_YEAR] = atoi(value);
    }
    found = domGetData(param, "max", value, 64);
    if (found == 1)
    {
        g_iMaxs[FIELD_YEAR] = atoi(value);
    }
    g_objFields[FIELD_YEAR] = CreateFieldLabel(Self(), x, y, g_szFields[FIELD_YEAR]);
    AnimColor(g_objFields[FIELD_YEAR], 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    g_iValues[FIELD_MONTH] = atoi(g_szFields[FIELD_MONTH]);
    g_iValues[FIELD_YEAR] = atoi(g_szFields[FIELD_YEAR]);
    g_iDigits[FIELD_MONTH] = 2;
    g_iDigits[FIELD_YEAR] = 4;
    CreateArrows();
}

/* Handles the messages and the blinks, then the pad while the date has the focus, by g_iMode. */
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
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the date the pad back. */
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
            for (new i; i < 2; i++)
            {
                AnimColor(g_objFields[i], 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
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

/* "FOUT" takes the focus away; any other dir gives it to the date. */
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
    send_focus2(g_objFocus, g_objPeriod, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    ShowPadHelp();
    g_bActive = 1;
}

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
}

/* Circle starts the edit on the field g_iField; cross and the directions go to the manager. */
UpdateNavigate()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_EDIT;
        send_label_focus2(g_objFocus, g_objFields[g_iField], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        for (new i; i < 2; i++)
        {
            g_iOldValues[i] = g_iValues[i];
        }
        new Float:x;
        GetPos(g_objFields[g_iField], x, _, _, 0);
        SetPos(g_objArrows[ARROW_UP], -2.0 + x, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 0);
        SetPos(g_objArrows[ARROW_DOWN], -2.0 + x, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 0);
        ShowArrows();
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

/*
 * Left and right choose the field, up and down change it within its range; circle confirms the date, cross restores it.
 */
UpdateEdit()
{
    new text[64];
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_NAVIGATE;
        for (new i; i < 2; i++)
        {
            g_iArrowBlinks[i] = BLINK_NONE;
            AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        send_focus2(g_objFocus, g_objPeriod, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        sprintf(g_szText, "%04d%02d", g_iValues[FIELD_YEAR], g_iValues);
        SendMessage(g_objManager, TAG("CHNG"), 0, 0, 0, 0);
        g_bActive = 0;
        play_se(ED_SE_DECIDE);
    }
    else if (g_iPressed & PAD_CROSS)
    {
        g_iMode = MODE_NAVIGATE;
        for (new i; i < 2; i++)
        {
            g_iArrowBlinks[i] = BLINK_NONE;
            AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        for (new i; i < 2; i++)
        {
            g_iValues[i] = g_iOldValues[i];
            if (0 == g_iDigits[i])
            {
                copy_string(text, "%d", 64);
            }
            else
            {
                sprintf(text, "%%0%dd", g_iDigits[i]);
            }
            sprintf(text, text, g_iValues[i]);
            SetOptionS(g_objFields[i], TAG("UTF8"), text);
        }
        send_focus2(g_objFocus, g_objPeriod, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        play_se(ED_SE_CANCEL);
    }
    else if (g_iRepeated & PAD_RIGHT)
    {
        if (g_iField < FIELD_YEAR)
        {
            g_iField++;
            send_label_focus2(g_objFocus, g_objFields[g_iField], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            new Float:x;
            GetPos(g_objFields[g_iField], x, _, _, 0);
            AnimMoveLine(g_objArrows[ARROW_UP], -2.0 + x, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 100.0,
                    0.0, ANIM_FUNC_LINEAR);
            AnimMoveLine(g_objArrows[ARROW_DOWN], -2.0 + x, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 100.0, 0.0,
                    ANIM_FUNC_LINEAR);
            ShowArrows();
            play_se(ED_SE_CURSOR);
        }
    }
    else if (g_iRepeated & PAD_LEFT)
    {
        if (g_iField > FIELD_MONTH)
        {
            g_iField--;
            send_label_focus2(g_objFocus, g_objFields[g_iField], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            new Float:x;
            GetPos(g_objFields[g_iField], x, _, _, 0);
            AnimMoveLine(g_objArrows[ARROW_UP], -2.0 + x, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 100.0,
                    0.0, ANIM_FUNC_LINEAR);
            AnimMoveLine(g_objArrows[ARROW_DOWN], -2.0 + x, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 100.0, 0.0,
                    ANIM_FUNC_LINEAR);
            ShowArrows();
            play_se(ED_SE_CURSOR);
        }
    }
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iValues[g_iField] < g_iMaxs[g_iField])
        {
            g_iValues[g_iField]++;
            if (0 == g_iDigits[g_iField])
            {
                copy_string(text, "%d", 64);
            }
            else
            {
                sprintf(text, "%%0%dd", g_iDigits[g_iField]);
            }
            sprintf(text, text, g_iValues[g_iField]);
            SetOptionS(g_objFields[g_iField], TAG("UTF8"), text);
            if (g_iValues[g_iField] == g_iMaxs[g_iField])
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
        if (g_iValues[g_iField] > g_iMins[g_iField])
        {
            g_iValues[g_iField]--;
            if (0 == g_iDigits[g_iField])
            {
                copy_string(text, "%d", 64);
            }
            else
            {
                sprintf(text, "%%0%dd", g_iDigits[g_iField]);
            }
            sprintf(text, text, g_iValues[g_iField]);
            SetOptionS(g_objFields[g_iField], TAG("UTF8"), text);
            if (g_iValues[g_iField] == g_iMins[g_iField])
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

/* Creates the text of a field, centered at x, y, hidden. */
CreateFieldLabel(parent, Float:x, Float:y, text[])
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

/* Creates the arrows above and below the field, hidden. */
CreateArrows()
{
    g_objArrows[ARROW_UP] = CreatePlane(Self());
    SetPos(g_objArrows[ARROW_UP], -2.0, -1.0 * (2.0 + 0.5 * (8.0 + float(g_iFontSize))), 0.0, 0);
    SetScale(g_objArrows[ARROW_UP], 17.0, 8.0, 1.0);
    SetColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0);
    SetRot(g_objArrows[ARROW_UP], 0.0, 0.0, g_flPi, 0, 0);
    SetTexture(g_objArrows[ARROW_UP], imgGetResource(RES_SPIN), 0, -1);
    g_objArrows[ARROW_DOWN] = CreatePlane(Self());
    SetPos(g_objArrows[ARROW_DOWN], -2.0, 2.0 + 0.5 * (8.0 + float(g_iFontSize)), 0.0, 0);
    SetScale(g_objArrows[ARROW_DOWN], 17.0, 8.0, 1.0);
    SetColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objArrows[ARROW_DOWN], imgGetResource(RES_SPIN), 0, -1);
}

/* Shows the arrows of the field, without the one past its end. */
ShowArrows()
{
    if (g_iValues[g_iField] == g_iMaxs[g_iField])
    {
        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_iValues[g_iField] == g_iMins[g_iField])
    {
        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    }
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
