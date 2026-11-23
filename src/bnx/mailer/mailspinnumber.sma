/*
 * /opt0/bn/script/mailer/mailspinnumber.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A number of the mailer settings, changed with up and down between "min" and "max" (from "sel") under a
 * title ("label"), between the texts "top" and "end"; held, it speeds up by "spd" a second. With "flg"
 * the number is an index into the message sizes of g_iSizeSteps.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

new Float:g_flPi = 3.141592;

/* The arrows of g_objArrows, g_flArrowYs and g_iArrowBlinks. */
const ARROW_UP = 0;   /* above the number */
const ARROW_DOWN = 1; /* below the number */

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

new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flLabelRed = 0.215;
new Float:g_flLabelGreen = 0.708;
new Float:g_flLabelBlue = 0.999;
new Float:g_flDimAlpha = 0.2;
new g_iSizeSteps[7] = {8, 16, 32, 64, 128, 256, 512}; /* the choices of the largest message, in KB */
new g_iItemCount;
new g_szItemId[64];

/* Appends to ids, from g_iItemCount on, the id of every element below node with no children and no text. */
CollectItemIds(node, ids[][])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szItemId[0] = 0;
    domGetData(node, "id", g_szItemId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            sprintf(ids[g_iItemCount++], "%s", g_szItemId);
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            CollectItemIds(child, ids);
        }
    }
}

new g_iItemIndex;
new g_szFindId[64];

/* Counts the empty leaves below node like CollectItemIds; the one whose id is id sets g_iItemIndex. */
FindItemIndex(node, id[])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szFindId[0] = 0;
    domGetData(node, "id", g_szFindId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            if (!strcmp(g_szFindId, id))
            {
                g_iItemIndex = g_iItemCount;
            }
            g_iItemCount++;
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            FindItemIndex(child, id);
        }
    }
}

public g_manager; /* the settings page: gets "SHWT" when the spin is ready */
public g_focus; /* the focus object, moved onto the number */
public g_active; /* 1 while the spin has the focus and takes the pad */
public g_select; /* the value chosen */
public g_cursor; /* the number shown: the value, or its index with "flg" */
new g_iMaxValue;
new g_iMinValue;
new g_objTitle;
new g_objPrefix;
new g_objNumber;
new g_objSuffix;
new g_objArrows[2]; /* above and below the number */
new g_iArrowBlinks[2]; /* frames left of the blink of an arrow after a step */
new g_bUseSteps; /* argument "flg": "min" and "max" are indexes of g_iSizeSteps */

main()
{
    new value[256];
    new Float:pos[3];
    GetArg("max", value, 256);
    g_iMaxValue = atoi(value);
    GetArg("min", value, 256);
    g_iMinValue = atoi(value);
    GetArg("sel", value, 256);
    g_select = g_cursor = atoi(value);
    g_bUseSteps = 0;
    if (GetArg("flg", value, 256))
    {
        g_select = g_iSizeSteps[g_cursor];
        g_bUseSteps = 1;
    }
    get_group_tag_attr2("SEDIT", "HEAD", 0, "folder_x", value, 256);
    pos[0] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "folder_y", value, 256);
    pos[1] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "folder_z", value, 256);
    pos[2] = atof(value);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], Disp2D(0));
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    if (GetArg("label", value, 256))
    {
        SetOptionS(g_objTitle, TAG("UTF8"), value);
    }
    get_group_tag_attr2("SEDIT", "HEAD", 0, "spin_x", value, 256);
    pos[0] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "spin_y", value, 256);
    pos[1] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "spin_z", value, 256);
    pos[2] = atof(value);
    g_objPrefix = CreateLabel(Self());
    SetPos(g_objPrefix, pos[0] - 38.0, pos[1], pos[2], 0);
    SetColor(g_objPrefix, g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
    SetOptionI(g_objPrefix, TAG("HALN"), ALIGN_RIGHT);
    SetOptionI(g_objPrefix, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objPrefix, TAG("FSIZ"), 15);
    SetOptionI(g_objPrefix, TAG("ZSRT"), ZSORT_OVERLAY);
    if (GetArg("top", value, 256))
    {
        SetOptionS(g_objPrefix, TAG("UTF8"), value);
    }
    g_objNumber = CreateLabel(Self());
    SetPos(g_objNumber, pos[0], pos[1], pos[2], 0);
    SetColor(g_objNumber, g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
    SetOptionI(g_objNumber, TAG("HALN"), ALIGN_RIGHT);
    SetOptionI(g_objNumber, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objNumber, TAG("FSIZ"), 15);
    SetOptionI(g_objNumber, TAG("ZSRT"), ZSORT_OVERLAY);
    g_objSuffix = CreateLabel(Self());
    SetPos(g_objSuffix, pos[0], pos[1], pos[2], 0);
    SetColor(g_objSuffix, g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
    SetOptionI(g_objSuffix, TAG("HALN"), 4); /* not a horizontal alignment: the label is aligned left */
    SetOptionI(g_objSuffix, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objSuffix, TAG("FSIZ"), 15);
    SetOptionI(g_objSuffix, TAG("ZSRT"), ZSORT_OVERLAY);
    if (GetArg("end", value, 256))
    {
        SetOptionS(g_objSuffix, TAG("UTF8"), value);
    }
    ShowValue();
    ShowArrows();
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("BLNK"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        new held;
        new pressed;
        new repeated;
        if (g_active)
        {
            PadState(held, pressed, _, repeated, -1);
        }
        /* refocus: the focus must move to the number; held_count grows while the same buttons are held */
        static refocus;
        static last_held;
        static held_count;
        if (last_held == held)
        {
            copy_string(value, "", 256);
            GetArg("spd", value, 256);
            held_count = held_count + atoi(value);
        }
        else
        {
            held_count = 0;
        }
        last_held = held;
        new step = held_count / FrameRate() + 1;
        if (repeated & PAD_UP)
        {
            if (IncreaseValue(step))
            {
                BlinkArrow(ARROW_UP);
            }
            refocus = 1;
        }
        else if (repeated & PAD_DOWN)
        {
            if (DecreaseValue(step))
            {
                BlinkArrow(ARROW_DOWN);
            }
            refocus = 1;
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objNumber, 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /* "FOCS" IN/OUT, "FADE" IN/OUT/DIM. */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
                    g_active = 1;
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 0;
                    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else if (msg == TAG("BLNK"))
            {
                if (what == TAG("IN"))
                {
                }
                else if (what == TAG("OUT"))
                {
                }
            }
            else if (msg == TAG("FADE"))
            {
                if (what == TAG("IN"))
                {
                    FadeIn();
                }
                else if (what == TAG("OUT"))
                {
                    FadeOut();
                }
                else if (what == TAG("DIM"))
                {
                    FadeDim();
                }
            }
        }
        UpdateArrowBlinks();
        sleep;
    }
    return;
}

/* Fades the number and its texts to alpha. */
FadeLabels(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    GetColor(g_objNumber, color[0], color[1], color[2], color[3]);
    AnimColor(g_objNumber, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objPrefix, color[0], color[1], color[2], color[3]);
    AnimColor(g_objPrefix, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objSuffix, color[0], color[1], color[2], color[3]);
    AnimColor(g_objSuffix, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeLabels(1.0, float(400), 0.0);
}

/* Fades the spin out and removes the arrows. */
FadeOut()
{
    FadeLabels(0.0, float(400), 0.0);
    for (new i; i < 2; i++)
    {
        Destroy(g_objArrows[i]);
        g_objArrows[i] = 0;
    }
}

FadeDim()
{
    FadeLabels(g_flDimAlpha, float(400), 0.0);
}

ShowValue()
{
    new text[8];
    if (g_bUseSteps)
    {
        sprintf(text, "%d", g_iSizeSteps[g_cursor]);
    }
    else
    {
        sprintf(text, "%d", g_cursor);
    }
    SetOptionS(g_objNumber, TAG("UTF8"), text);
}

new Float:g_flArrowYs[2] = {-14.0, 14.0};

/* Adds step to the number, up to the maximum; returns 0 if it was already there. */
IncreaseValue(step)
{
    new moved = 1;
    g_cursor = g_cursor + step;
    if (g_cursor > g_iMaxValue)
    {
        g_cursor = g_iMaxValue;
        moved = 0;
    }
    else
    {
        sndSePlay(SE_CURSOR);
    }
    if (g_bUseSteps)
    {
        g_select = g_iSizeSteps[g_cursor];
    }
    else
    {
        g_select = g_cursor;
    }
    ShowValue();
    return moved;
}

/* Takes step from the number, down to the minimum; returns 0 if it was already there. */
DecreaseValue(step)
{
    new moved = 1;
    g_cursor = g_cursor - step;
    if (g_cursor < g_iMinValue)
    {
        g_cursor = g_iMinValue;
        moved = 0;
    }
    else
    {
        sndSePlay(SE_CURSOR);
    }
    if (g_bUseSteps)
    {
        g_select = g_iSizeSteps[g_cursor];
    }
    else
    {
        g_select = g_cursor;
    }
    ShowValue();
    return moved;
}

/* Creates the arrows above and below the number if needed and fades them in. */
ShowArrows()
{
    if (!g_objArrows[ARROW_UP])
    {
        g_objArrows[ARROW_UP] = CreatePlane(Self());
        SetPos(g_objArrows[ARROW_UP], -10.0, g_flArrowYs[ARROW_UP], 0.0, g_objNumber);
        SetScale(g_objArrows[ARROW_UP], 17.0, 8.0, 1.0);
        SetColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0);
        SetRot(g_objArrows[ARROW_UP], 0.0, 0.0, g_flPi, 0, 0);
        SetTexture(g_objArrows[ARROW_UP], imgGetResource(RES_SPIN), 0, -1);
    }
    if (!g_objArrows[ARROW_DOWN])
    {
        g_objArrows[ARROW_DOWN] = CreatePlane(Self());
        SetPos(g_objArrows[ARROW_DOWN], -10.0, g_flArrowYs[ARROW_DOWN], 0.0, g_objNumber);
        SetScale(g_objArrows[ARROW_DOWN], 17.0, 8.0, 1.0);
        SetColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 0.0);
        SetTexture(g_objArrows[ARROW_DOWN], imgGetResource(RES_SPIN), 0, -1);
    }
    AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, float(400), 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, float(400), 0.0, ANIM_FUNC_LINEAR);
}

/* Starts the blink of arrow index (ARROW_UP or ARROW_DOWN) after a step. */
BlinkArrow(index)
{
    if (!g_iArrowBlinks[index])
    {
        index = clamp(index, 0, 1);
        g_iArrowBlinks[index] = ms_to_frames(100) * 2;
        AnimColor(g_objArrows[index], 1.0, 1.0, 1.0, 0.0, float(100), 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Advances the blinks of the arrows by a frame. */
UpdateArrowBlinks()
{
    for (new i; i < 2; i++)
    {
        if (g_objArrows[i] && g_iArrowBlinks[i])
        {
            g_iArrowBlinks[i]--;
            if (g_iArrowBlinks[i] == ms_to_frames(100))
            {
                AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 1.0, float(100), 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
}
