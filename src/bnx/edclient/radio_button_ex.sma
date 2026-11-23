/*
 * /opt0/bn/script/edclient/radio_button_ex.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A row of choices of an edclient page, one for each "data" child of its parts element, with a mark on the
 * selected one. Circle starts a choice with left and right; circle again tells the manager "CHNG".
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
    MODE_NAVIGATE, /* the focus is on the row */
    MODE_CHOOSE,   /* the focus is on a choice */
};

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
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

public g_objManager; /* the page: gets "CHNG" with the choice, "PUSH" for cross, "FOCS" when the focus leaves */
public g_objFocus; /* the focus object, moved onto the row or the choice */
public g_tagParts; /* the element of the row: "fontsize", "interval", a "data" child with a "text_id" per choice */
public g_iCurrent; /* the choice selected first */
new g_bActive; /* the row has the focus and takes the pad */
new g_iMode;
new g_iSelected;
new g_iCount;
new g_iFontSize = 15;
new Float:g_flSpeed = 200.0; /* the time of the fade in, in ms */
new Float:g_flInterval = 30.0; /* between two choices */
new g_objRow; /* an empty object as large as the row, for the focus */
new g_objMarks[10];
new g_objLabels[10];
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

/* Reads "fontsize" and "interval", creates a mark and a text for each choice and lays them out in a row. */
Init()
{
    new value[64];
    new text_id[64];
    new node;
    new len;
    g_iSelected = g_iCurrent;
    len = domGetData(g_tagParts, "fontsize", value, 64);
    if (len == 1)
    {
        g_iFontSize = atoi(value);
    }
    len = domGetData(g_tagParts, "interval", value, 64);
    if (len == 1)
    {
        g_flInterval = atof(value);
    }
    g_iCount = 0;
    node = domFirstChild(g_tagParts, "");
    for (; node; node = domNext(node, ""))
    {
        domGetTagName(node, value, 64);
        if (0 != strcmp("data", value))
        {
            continue;
        }
        len = domGetData(node, "text_id", text_id, 64);
        value = "";
        get_tag_attr("TEXT", text_id, "value", value, 64);
        g_objLabels[g_iCount] = CreateChoiceLabel(Self(), 0.0, 0.0, value);
        g_objMarks[g_iCount] = CreateMark(Self(), 0.0, 0.0);
        g_iCount++;
    }
    clamp(g_iSelected, 0, g_iCount - 1); /* NOTE: bug: the result is dropped, g_iSelected is not clamped */
    new Float:label_w[10];
    for (new i; i < g_iCount; )
    {
        new rect[5];
        GetOptionP(g_objLabels[i], TAG("RECT"), rect, 5);
        if (rect[0])
        {
            label_w[i] = float(rect[3]) / 64.0;
            i++;
        }
        sleep;
    }
    {
        new Float:x = 18.0 * 0.5;
        new Float:row_w = float(g_iFontSize);
        new Float:row_h = float(g_iFontSize);
        for (new i; i < g_iCount; i++)
        {
            SetPos(g_objMarks[i], x, 0.0, 0.0, 0);
            x = 18.0 + x;
            SetPos(g_objLabels[i], x, 0.0, 0.0, 0);
            x = x + label_w[i];
            x = x + g_flInterval;
        }
        x = x - g_flInterval;
        row_w = x;
        x = 0.5 * row_w;
        g_objRow = CreateNull(Self());
        SetPos(g_objRow, x, 0.0, 0.0, 0);
        SetLocalScale(g_objRow, row_w, row_h, 1.0);
    }
    {
        new Float:alpha;
        for (new i; i < g_iCount; i++)
        {
            AnimColor(g_objLabels[i], 0.215, 0.708, 0.999, 1.0, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
            alpha = g_iSelected == i ? 1.0 : 0.0;
            AnimColor(g_objMarks[i], 1.0, 1.0, 1.0, alpha, g_flSpeed, 0.0, ANIM_FUNC_LINEAR);
        }
    }
}

/* Handles the messages, then the pad while the row has the focus, by g_iMode. */
Update()
{
    ProcessMessages();
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
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the row the pad back. */
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
            for (new i; i < g_iCount; i++)
            {
                AnimColor(g_objLabels[i], 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_objMarks[i], 1.0, 1.0, 1.0, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
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
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the row. */
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
    send_focus(g_objFocus, g_objRow, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    g_bActive = 1;
}

/* Circle starts a choice; cross and the directions go to the manager. */
UpdateNavigate()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_CHOOSE;
        send_label_focus2(g_objFocus, g_objLabels[g_iSelected], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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

/* Circle confirms the choice and moves the mark, cross gives up the choice. */
UpdateChoose()
{
    if (g_iPressed & PAD_CIRCLE)
    {
        g_iMode = MODE_NAVIGATE;
        send_focus(g_objFocus, g_objRow, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        {
            new Float:alpha;
            for (new i; i < g_iCount; i++)
            {
                alpha = g_iSelected == i ? 1.0 : 0.0;
                AnimColor(g_objMarks[i], 1.0, 1.0, 1.0, alpha, 400.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        SendMessage(g_objManager, TAG("CHNG"), g_iSelected, 0, 0, 0);
        g_bActive = 0;
        play_se(ED_SE_DECIDE);
    }
    else if (g_iPressed & PAD_CROSS)
    {
        g_iMode = MODE_NAVIGATE;
        send_focus(g_objFocus, g_objRow, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        play_se(ED_SE_CANCEL);
    }
    else if (g_iRepeated & PAD_RIGHT)
    {
        if (g_iSelected < g_iCount - 1)
        {
            g_iSelected = g_iSelected < g_iCount - 1 ? ++g_iSelected : g_iSelected; /* redundant as written */
            send_label_focus2(g_objFocus, g_objLabels[g_iSelected], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
    }
    else if (g_iRepeated & PAD_LEFT)
    {
        if (g_iSelected > 0)
        {
            g_iSelected = g_iSelected > 0 ? --g_iSelected : g_iSelected; /* redundant as written */
            send_label_focus2(g_objFocus, g_objLabels[g_iSelected], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        }
    }
    else if (g_iRepeated & PAD_UP)
    {
    }
    else if (g_iRepeated & PAD_DOWN)
    {
    }
}

/* Creates the text of a choice, aligned left at x, y, hidden. */
CreateChoiceLabel(parent, Float:x, Float:y, text[])
{
    new label;
    label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetColor(label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), g_iFontSize);
    SetOptionS(label, TAG("UTF8"), text);
    return label;
}

/* Creates the mark of a choice (RES_RADIO) at x, y, hidden. */
CreateMark(parent, Float:x, Float:y)
{
    new mark;
    mark = CreatePlane(parent);
    SetPos(mark, x, y, 0.0, 0);
    SetScale(mark, 18.0, 16.0, 1.0);
    SetColor(mark, 1.0, 1.0, 1.0, 0.0);
    SetTexture(mark, imgGetResource(RES_RADIO), 0, -1);
    SetOptionI(mark, TAG("ALPH"), BLEND_ADD);
    return mark;
}
