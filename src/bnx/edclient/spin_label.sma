/*
 * /opt0/bn/script/edclient/spin_label.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A choice of an edclient form shown one at a time, from the "data" children of its parts element: circle starts
 * the choice, up and down scroll the texts, circle confirms it ("CHNG" to the manager) and cross gives it up.
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
    MODE_NAVIGATE, /* the focus is on the choice */
    MODE_CHOOSE,   /* up and down scroll the choices */
};

/* Values of g_iScroll: the scroll that runs. */
enum
{
    SCROLL_NONE,
    SCROLL_DOWN, /* a text added at the bottom: the top one goes when it ends */
    SCROLL_UP,
};

/* The arrows of g_objArrows and g_iArrowBlinks. */
const ARROW_UP = 0;   /* above the choice */
const ARROW_DOWN = 1; /* below the choice */

/* Values of g_iArrowBlinks: an arrow blinks when the choice moves away from its end. */
enum
{
    BLINK_NONE,
    BLINK_FADE_IN,
    BLINK_FADE_OUT,
};

new Float:g_flPi = 3.141592;

#include "text.inc"

#include "misc.inc"

new g_any_child3[1]; /* empty: any element */
new g_any_next3[1]; /* empty: any element */

#include "dom.inc"

#include "background.inc"

public g_objManager; /* the page: gets "CHNG" with the choice, "PUSH" for cross, "FOCS" when the focus leaves */
public g_objFocus; /* the focus object, moved onto the choice */
public g_tagParts; /* the element of the choice: a "data" child with a "text_id" for each choice */
public g_iCurrent; /* the choice selected first */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the choice has the focus */
new g_bActive; /* the choice has the focus and takes the pad */
new g_iMode;
new g_iScroll;
new g_iFontSize = 15;
new Float:g_flSpeed = 200.0; /* the time of a fade and of the slowest scroll, in ms */
new g_iSelected;
new g_iCount;
new g_objLabel; /* the choice confirmed; the list holds the texts while choosing */
new g_tagData; /* the "data" element of g_iSelected */
new g_iScrollTick; /* the tickcount of the last scroll: a scroll lasts the time since the one before */
new g_iPrevTick;
new g_iMaxTime;
new g_iMinTime;
new g_objTimer; /* the timer of the scroll */
new g_iArrowBlinks[2];
new g_objArrows[2]; /* above and below the choice */
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

/* Counts the choices and creates the text of the one of g_iCurrent and the arrows. */
Init()
{
    new text[64];
    new node;
    SystemCommand(SYSCMD_WIPE_ON_FREE);
    node = domFirstChild(g_tagParts, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
    {
        domGetTagName(node, text, 64);
        if (0 != strcmp("data", text))
        {
            continue;
        }
        g_iCount++;
    }
    g_iSelected = clamp(g_iCurrent, 0, g_iCount - 1);
    g_tagData = find_nth_child_tag2(g_tagParts, "data", g_iSelected);
    domGetData(g_tagData, "text_id", text, 64);
    get_tag_attr2("TEXT", text, "value", text, 64);
    g_objLabel = CreateChoiceLabel(Self(), 0.0, 0.0, text);
    AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
    g_iMaxTime = floatround(g_flSpeed, 0);
    g_iMinTime = g_iMaxTime < 60 ? g_iMaxTime : 60;
    g_objTimer = CreateNull(Self());
    CreateArrows();
}

/* Handles the messages, the scroll and the blinks, then the pad while the choice has the focus, by g_iMode. */
Update()
{
    ProcessMessages();
    UpdateScroll();
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
        case MODE_CHOOSE:
        {
            UpdateChoose();
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
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the choice the pad back. */
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
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the choice. */
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

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
}

/* Circle starts the choice, with the texts in the list; cross and the directions go to the manager. */
UpdateNavigate()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_CHOOSE;
        AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        for (new i; i < listSize(); i++)
        {
            Destroy(listGetAt(i));
        }
        listRemoveAll();
        g_tagData = find_nth_child_tag2(g_tagParts, "data", g_iSelected);
        new text[64];
        new label;
        domGetData(g_tagData, "text_id", text, 64);
        get_tag_attr2("TEXT", text, "value", text, 64);
        label = CreateChoiceLabel(Self(), 0.0, 0.0, text);
        AnimColor(label, 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        listAddLast(label);
        send_label_focus2(g_objFocus, listGetAt(0), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (g_iSelected > 0)
        {
            AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        if (g_iSelected < g_iCount - 1)
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

/* Circle confirms the choice, cross gives it up; up and down scroll to the next text. */
UpdateChoose()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_NAVIGATE;
        for (new i; i < listSize(); i++)
        {
            AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        new text[64];
        domGetData(g_tagData, "text_id", text, 64);
        get_tag_attr2("TEXT", text, "value", text, 64);
        SetOptionS(g_objLabel, TAG("UTF8"), text);
        AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0 * 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        send_label_focus2(g_objFocus, g_objLabel, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        for (new i; i < 2; i++)
        {
            g_iArrowBlinks[i] = BLINK_NONE;
            AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        SendMessage(g_objManager, TAG("CHNG"), g_iSelected, 0, 0, 0);
        g_bActive = 0;
        play_se(ED_SE_DECIDE);
    }
    else if (g_iPressed & PAD_CROSS)
    {
        g_iMode = MODE_NAVIGATE;
        for (new i; i < listSize(); i++)
        {
            AnimColor(listGetAt(i), 0.215, 0.708, 0.999, 1.0 * 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        AnimColor(g_objLabel, 0.215, 0.708, 0.999, 1.0 * 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        send_label_focus2(g_objFocus, g_objLabel, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        for (new i; i < 2; i++)
        {
            g_iArrowBlinks[i] = BLINK_NONE;
            AnimColor(g_objArrows[i], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
        play_se(ED_SE_CANCEL);
    }
    else if (g_iRepeated & PAD_RIGHT)
    {
    }
    else if (g_iRepeated & PAD_LEFT)
    {
    }
    else if (g_iRepeated & PAD_UP)
    {
        if (g_iSelected > 0)
        {
            g_iSelected = g_iSelected > 0 ? --g_iSelected : g_iSelected; /* redundant as written */
            FinishScroll();
            new scroll_time = GetScrollTime();
            g_tagData = domPrev(g_tagData, "");
            AnimColor(listGetLast(), 0.215, 0.708, 0.999, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            new Float:y;
            new text[64];
            new label;
            y = -1.0 * float(g_iFontSize);
            domGetData(g_tagData, "text_id", text, 64);
            get_tag_attr2("TEXT", text, "value", text, 64);
            label = CreateChoiceLabel(Self(), 0.0, 0.0, text);
            SetPos(label, 0.0, y, 0.0, 0);
            AnimColor(label, 0.215, 0.708, 0.999, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            listInsert(0, label);
            y = 0.0;
            for (new i; i < listSize(); i++)
            {
                AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                y = y + float(g_iFontSize);
            }
            g_iScroll = SCROLL_UP;
            AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            if (0 == g_iSelected)
            {
                g_iArrowBlinks[ARROW_UP] = BLINK_NONE;
                AnimColor(g_objArrows[ARROW_UP], 1.0, 1.0, 1.0, 0.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                BlinkArrow(ARROW_UP);
            }
            AnimColor(g_objArrows[ARROW_DOWN], 1.0, 1.0, 1.0, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
            send_label_focus2(g_objFocus, listGetAt(0), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            play_se(ED_SE_CURSOR);
        }
    }
    else if (g_iRepeated & PAD_DOWN)
    {
        if (g_iSelected < g_iCount - 1)
        {
            g_iSelected = g_iSelected < g_iCount - 1 ? ++g_iSelected : g_iSelected; /* redundant as written */
            FinishScroll();
            new scroll_time = GetScrollTime();
            g_tagData = domNext(g_tagData, g_any_next3);
            AnimColor(listGetAt(0), 0.215, 0.708, 0.999, 1.0 * 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            new Float:y;
            new text[64];
            new label;
            y = float(g_iFontSize);
            domGetData(g_tagData, "text_id", text, 64);
            get_tag_attr2("TEXT", text, "value", text, 64);
            label = CreateChoiceLabel(Self(), 0.0, 0.0, text);
            SetPos(label, 0.0, y, 0.0, 0);
            AnimColor(label, 0.215, 0.708, 0.999, 1.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            listAddLast(label);
            y = -1.0 * float(g_iFontSize);
            for (new i; i < listSize(); i++)
            {
                AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
                y = y + float(g_iFontSize);
            }
            g_iScroll = SCROLL_DOWN;
            AnimTimerManager(g_objTimer, float(scroll_time), 0.0, ANIM_FUNC_LINEAR);
            send_label_focus2(g_objFocus, listGetLast(), 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (g_iSelected == g_iCount - 1)
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

/* Creates the text of a choice, centered at x, y, hidden. */
CreateChoiceLabel(parent, Float:x, Float:y, text[])
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

/* Returns the time of a scroll: the time since the last one, from g_iMinTime to g_iMaxTime. */
GetScrollTime()
{
    new scroll_time;
    g_iPrevTick = g_iScrollTick;
    g_iScrollTick = tickcount();
    scroll_time = g_iScrollTick - g_iPrevTick;
    scroll_time = clamp(scroll_time, g_iMinTime, g_iMaxTime);
    return scroll_time;
}

/* Removes the text scrolled out when the scroll ends. */
UpdateScroll()
{
    if (SCROLL_NONE == g_iScroll)
    {
        return;
    }
    switch (AnimGetState(g_objTimer, ANIM_TIMER))
    {
        case ANIM_DONE:
        {
            if (g_iScroll == SCROLL_DOWN)
            {
                Destroy(listGetAt(0));
                listRemoveAt(0);
            }
            else if (g_iScroll == SCROLL_UP)
            {
                Destroy(listGetLast());
                listRemoveLast();
            }
            g_iScroll = SCROLL_NONE;
        }
        case ANIM_DELAY:
        {
        }
        case ANIM_RUN:
        {
        }
        default:
        {
        }
    }
}

/* Removes the text scrolled out at once, before a new scroll. */
FinishScroll()
{
    if (SCROLL_NONE == g_iScroll)
    {
        return;
    }
    if (ANIM_DONE != AnimGetState(g_objTimer, ANIM_TIMER))
    {
        if (g_iScroll == SCROLL_DOWN)
        {
            Destroy(listGetAt(0));
            listRemoveAt(0);
        }
        else if (g_iScroll == SCROLL_UP)
        {
            Destroy(listGetLast());
            listRemoveLast();
        }
        g_iScroll = SCROLL_NONE;
    }
}

/* Creates the arrows above and below the choice, hidden. */
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
