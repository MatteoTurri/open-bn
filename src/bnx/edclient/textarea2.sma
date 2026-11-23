/*
 * /opt0/bn/script/edclient/textarea2.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A read-only text box of an edclient page: the TEXT element of argument "text_id", scrolled a line at a time
 * with up and down, with a scroll bar on its left. Circle and cross go to the manager as "PUSH".
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

#include "tag.inc"

#include <float>

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

#include "background.inc"

public g_objManager; /* the page: gets "PUSH" for circle and cross, "FOCS" for right and left */
public g_objFocus; /* the focus object, moved onto the box */
public g_iFontSize = 15; /* the font size of the text, in pixels ("FSIZ") */
public g_iWidth; /* in pixels */
public g_iHeight; /* in pixels */
public g_iPadHelp = PADHELP_CIRCLE | PADHELP_CROSS; /* the SYS_PADHELP buttons while the box has the focus */
new g_bActive; /* the box has the focus and takes the pad */
new g_iTopLine; /* the first line shown */
new g_iLineCount; /* lines of the whole text */
new g_iPageLines; /* g_iBoxLines, at most g_iLineCount */
new g_objText;
new g_szText[12288];
new g_iBoxLines; /* lines the box can show */
new g_objScrollBar;

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/* Reads the text of the TEXT element of argument "text_id" and creates the box and its scroll bar. */
Init()
{
    new text_id[64];
    new found;
    found = GetArg("text_id", text_id, 64);
    if (found == 1)
    {
        new node = domFindTag(domRoot(), "TEXT", text_id, 0);
        domGetData(node, "", g_szText, 12288);
    }
    g_objText = CreateText();
    g_objScrollBar = CreateScrollBar();
}

/* Handles the messages, then the pad while the box has the focus: up and down scroll a line. */
Update()
{
    ProcessMessages();
    if (0 == g_bActive)
    {
        return;
    }
    new repeated;
    new pressed;
    PadState(_, pressed, _, repeated, -1);
    if (pressed & PAD_CIRCLE)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        g_bActive = 0;
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
    else if (repeated & PAD_UP)
    {
        if (g_iTopLine > 0)
        {
            g_iTopLine = g_iTopLine > 0 ? --g_iTopLine : g_iTopLine; /* redundant as written */
            SetOptionI(g_objText, TAG("SCLY"), -1);
            UpdateScrollBar();
            play_se(ED_SE_CURSOR);
        }
    }
    else if (repeated & PAD_DOWN)
    {
        if (g_iTopLine < g_iLineCount - g_iPageLines)
        {
            /* redundant as written */
            g_iTopLine = g_iTopLine < g_iLineCount - g_iPageLines ? ++g_iTopLine : g_iTopLine;
            SetOptionI(g_objText, TAG("SCLY"), 1);
            UpdateScrollBar();
            play_se(ED_SE_CURSOR);
        }
    }
}

ProcessMessages()
{
    new msg;
    new a;
    new Float:b;
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" gives the box the pad back. */
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
            AnimColor(g_objText, 1.0, 1.0, 1.0, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            SendMessage(g_objScrollBar, TAG("FADE"), a, b, 0, 0);
        }
        else if (msg == TAG("PUSH"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("CHNG"))
        {
        }
        else if (msg == TAG("GTXT"))
        {
        }
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the box. */
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
    send_focus(g_objFocus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    ShowPadHelp();
    g_bActive = 1;
}

/* Shows the buttons of g_iPadHelp in the pad help. */
ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, g_iPadHelp);
}

/* Creates the read-only text in a g_iWidth by g_iHeight box and counts its lines and the lines it shows. */
CreateText()
{
    new text;
    text = CreateEditML(Self());
    SetPos(text, 0.0, 0.0, 0.0, 0);
    SetColor(text, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(text, TAG("FSIZ"), g_iFontSize);
    SetOptionI(text, TAG("W64 "), g_iWidth * 64);
    SetOptionI(text, TAG("H64 "), g_iHeight * 64);
    SetOptionI(text, TAG("CRSW"), 0);
    SetOptionI(text, TAG("EDTE"), 0);
    SetOptionI(text, TAG("SDWE"), 1);
    SetOptionI(text, TAG("CSPC"), 0);
    SetOptionI(text, TAG("LSPC"), 0);
    SetOptionI(text, TAG("CPDY"), g_iTopLine);
    SetOptionS(text, TAG("UTF8"), g_szText);
    {
        new packed[4];
        static bg_bytes[16];
        pack_color_bytes(packed, bg_bytes);
        SetOptionP(text, TAG("BGC4"), packed, 4);
        static color_bytes[16] = {255, 255, 255, 200, 255, 255, 255, 200, 255, 255, 255, 200, 255, 255, 255, 200};
        pack_color_bytes(packed, color_bytes);
        SetOptionP(text, TAG("COLR"), packed, 4);
    }
    GetOptionI(text, TAG("CPDM"), g_iLineCount);
    GetOptionI(text, TAG("CPDL"), g_iPageLines);
    g_iBoxLines = g_iPageLines;
    g_iPageLines = g_iPageLines > g_iLineCount ? g_iLineCount : g_iPageLines;
    AnimColor(text, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    return text;
}

/* Creates the scroll bar (script scroll_bar) on the left of the box, its thumb as long as the part shown. */
CreateScrollBar()
{
    new bar;
    new Float:bar_h;
    new Float:length;
    new Float:top;
    bar_h = float(g_iHeight);
    bar = CreateNull(Self());
    SetPos(bar, -16.0, 0.5 * bar_h, 0.0, 0);
    AttachScript(bar, _, "scroll_bar");
    SetPubVar(bar, "manager", Self());
    SetPubVar(bar, "focus", g_objFocus);
    SetPubVar(bar, "height", bar_h);
    length = float(g_iBoxLines) / float(g_iLineCount);
    length = length > 1.0 ? 1.0 : length;
    length = length < 0.0 ? 0.0 : length;
    length = length < 0.1 ? 0.1 : length;
    top = float(g_iTopLine) / float(g_iLineCount - g_iBoxLines) * (1.0 - length);
    SetPubVar(bar, "top", top);
    SetPubVar(bar, "length", length);
    return bar;
}

/* Moves the thumb of the scroll bar to g_iTopLine. */
UpdateScrollBar()
{
    new Float:length;
    new Float:top;
    length = float(g_iBoxLines) / float(g_iLineCount);
    length = length > 1.0 ? 1.0 : length;
    length = length < 0.0 ? 0.0 : length;
    length = length < 0.1 ? 0.1 : length;
    top = float(g_iTopLine) / float(g_iLineCount - g_iBoxLines) * (1.0 - length);
    SetPubVar(g_objScrollBar, "top", top);
}
