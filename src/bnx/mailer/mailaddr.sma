/*
 * /opt0/bn/script/mailer/mailaddr.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The address book of the mailer, opened by main: seven lines of names and addresses with a scroll bar, to edit
 * the entries or to pick one for the mail being written.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "mailer.inc"

#include "tag.inc"

#include <float>

/* The labels of g_objLabels. */
const LABEL_NAME = 0;    /* the header of the names */
const LABEL_ADDRESS = 1; /* the header of the addresses */
const LABEL_POSITION = 2;

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

#include "text.inc"

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flDimAlpha = 0.2;
new Float:g_flBlinkOutTime = 1200.0;
new Float:g_flBlinkInTime = 600.0;
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

public g_manager; /* the mailer page: holds the textures, gets "SHWT" when ready and "ACWT" after "ACT" */
public g_focus; /* the focus object */
public g_active; /* the page reads the pad */
public g_count; /* entries in the address book */
public g_line; /* the row of the cursor */
public g_top; /* the entry on the first row */
public g_mode; /* ADDR_MODE_* */
public g_blink; /* the cursor blinks */
public g_lock; /* unused, but part of the layout */
new g_objTitle;
new g_objCursor;
new g_objLabels[3]; /* the name and address headers, and the position */
new g_objScrollBar;
new g_objHeaderBg;
new g_objScrollTimer;
new g_iScrollDir;
new g_iScrollCount; /* the lines added by the scroll in progress */
new g_bBlinkOn;

/* Creates a line of the address book (mailaline) under parent. */
CreateLine(parent)
{
    new tex = GetPubVar(g_manager, "g_txAddrGrid");
    new Float:tex_w = GetPubVar(g_manager, "g_AddrGridWidth");
    new Float:tex_h = GetPubVar(g_manager, "g_AddrGridHeight");
    new line = CreatePlane(parent);
    SetColor(line, 0.0, 0.0, 0.0, 0.0);
    SetLocalScale(line, tex_w, tex_h, 0.0);
    SetOptionI(line, TAG("ALPH"), BLEND_ADD);
    SetTexture(line, tex, 0, -1);
    AttachScript(line, _, "mailaline", "");
    SetPubVar(line, "g_manager", g_manager);
    SetPubVar(line, "g_focus", g_focus);
    return line;
}

main()
{
    new value[16];
    new Float:pos[3];
    new Float:list_pos[3];
    new Float:width;
    new Float:height;
    new Float:line_h;
    listRemoveAll();
    g_iScrollDir = SCROLL_NONE;
    if (GetArg("line", value, 16))
    {
        g_line = atoi(value);
    }
    if (GetArg("top", value, 16))
    {
        g_top = atoi(value);
    }
    if (GetArg("mode", value, 16))
    {
        g_mode = atoi(value);
    }
    get_group_tag_attr2("ADDR", "HEAD", 0, "list_x", value, 16);
    list_pos[0] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "list_y", value, 16);
    list_pos[1] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "list_z", value, 16);
    list_pos[2] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "line_height", value, 16);
    line_h = atof(value);
    for (new i; i < LIST_LINES; i++)
    {
        new line = CreateLine(Self());
        SetPos(line, list_pos[0], list_pos[1] + line_h * i, list_pos[2], 0);
        listAddLast(line);
    }
    get_group_tag_attr2("ADDR", "HEAD", 0, "bar_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "bar_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "bar_z", value, 16);
    pos[2] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "bar_h", value, 16);
    height = atof(value);
    g_objScrollBar = CreateNull(Self());
    SetPos(g_objScrollBar, pos[0], pos[1], pos[2], 0);
    SetOptionI(g_objScrollBar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_objScrollBar, _, "scroll_bar", "");
    SetPubVar(g_objScrollBar, "manager", Self());
    SetPubVar(g_objScrollBar, "focus", g_focus);
    SetPubVar(g_objScrollBar, "height", height);
    SendMessage(g_objScrollBar, TAG("FADE"), 0, 0, 0, 0);
    get_group_tag_attr2("ADDR", "HEAD", 0, "folder_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "folder_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "folder_z", value, 16);
    pos[2] = atof(value);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    if (ADDR_MODE_BOOK == g_mode)
    {
        get_tag_attr("TEXT", "msgmailer_addredit_title", "value", value, 16);
        SetOptionS(g_objTitle, TAG("UTF8"), value);
    }
    else
    {
        get_tag_attr("TEXT", "msgmailer_addrref_title", "value", value, 16);
        SetOptionS(g_objTitle, TAG("UTF8"), value);
    }
    new header_tex = GetPubVar(g_manager, "g_txAddrTop");
    get_group_tag_attr2("ADDR", "HEAD", 0, "index_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "index_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "index_z", value, 16);
    pos[2] = atof(value);
    width = GetPubVar(g_manager, "g_AddrTopWidth");
    height = GetPubVar(g_manager, "g_AddrTopHeight");
    g_objHeaderBg = CreatePlane(Self());
    SetPos(g_objHeaderBg, pos[0], pos[1], pos[2], 0);
    SetScale(g_objHeaderBg, 2.0 + width, 24.0, 0.0);
    SetColor(g_objHeaderBg, 0.0, 0.0, 0.5, 0.0);
    SetOptionI(g_objHeaderBg, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objHeaderBg, header_tex, 0, -1);
    SetOptionI(g_objHeaderBg, TAG("ALPH"), BLEND_ADD);
    get_tag_attr("TEXT", "msgmailer_addrlist_name", "value", value, 16);
    g_objLabels[LABEL_NAME] = CreateLabel(Self());
    SetColor(g_objLabels[LABEL_NAME], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionS(g_objLabels[LABEL_NAME], TAG("UTF8"), value);
    SetOptionI(g_objLabels[LABEL_NAME], TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objLabels[LABEL_NAME], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objLabels[LABEL_NAME], TAG("FSIZ"), 15);
    SetOptionI(g_objLabels[LABEL_NAME], TAG("ZSRT"), ZSORT_OVERLAY);
    SetPos(g_objLabels[LABEL_NAME], 10.0 + (pos[0] - width / 2), 2.0 + pos[1], 0.0, 0);
    get_tag_attr("TEXT", "msgmailer_addrlist_addr", "value", value, 16);
    g_objLabels[LABEL_ADDRESS] = CreateLabel(Self());
    SetColor(g_objLabels[LABEL_ADDRESS], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionS(g_objLabels[LABEL_ADDRESS], TAG("UTF8"), value);
    SetOptionI(g_objLabels[LABEL_ADDRESS], TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objLabels[LABEL_ADDRESS], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objLabels[LABEL_ADDRESS], TAG("FSIZ"), 15);
    SetOptionI(g_objLabels[LABEL_ADDRESS], TAG("ZSRT"), ZSORT_OVERLAY);
    SetPos(g_objLabels[LABEL_ADDRESS], 230.0 + (pos[0] - width / 2), 2.0 + pos[1], 0.0, 0);
    get_group_tag_attr2("ADDR", "HEAD", 0, "addr_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "addr_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("ADDR", "HEAD", 0, "addr_z", value, 16);
    pos[2] = atof(value);
    g_objLabels[LABEL_POSITION] = CreateLabel(Self());
    SetColor(g_objLabels[LABEL_POSITION], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objLabels[LABEL_POSITION], TAG("HALN"), ALIGN_RIGHT);
    SetOptionI(g_objLabels[LABEL_POSITION], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objLabels[LABEL_POSITION], TAG("FSIZ"), 13);
    SetOptionI(g_objLabels[LABEL_POSITION], TAG("ZSRT"), ZSORT_OVERLAY);
    SetPos(g_objLabels[LABEL_POSITION], pos[0], pos[1], pos[2], 0);
    new cursor_tex = GetPubVar(g_manager, "g_txListFg");
    width = GetPubVar(g_manager, "g_AddrGridWidth");
    height = GetPubVar(g_manager, "g_AddrGridHeight");
    g_objCursor = CreatePlane(Self());
    SetScale(g_objCursor, width, height, 0.0);
    SetColor(g_objCursor, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objCursor, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_objCursor, TAG("ZOFS"), -1.0);
    SetOptionI(g_objCursor, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_objCursor, cursor_tex, 0, -1);
    Hide(g_objCursor);
    g_objScrollTimer = CreateNull(Self());
    LoadList(g_top, g_line);
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        new held;
        new pressed;
        new released;
        new repeated;
        if (g_active)
        {
            PadState(held, pressed, released, repeated, -1);
        }
        new old_line = g_line;
        if (held & (PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE))
        {
            sleep;
            continue;
        }
        else
        {
            static last_tick;
            if (pressed & PAD_L1 || (repeated & PAD_L1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                if (g_top + g_line > 0 && SCROLL_NONE == g_iScrollDir)
                {
                    sndSePlay(SE_CURSOR);
                    if (g_top + g_line - LIST_LINES > LIST_LINES)
                    {
                        g_top = g_top - LIST_LINES;
                    }
                    else
                    {
                        g_top = g_line = 0;
                    }
                    LoadList(g_top, g_line);
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                }
            }
            else if (pressed & PAD_R1 || (repeated & PAD_R1 && tickcount() - last_tick > 70))
            {
                last_tick = tickcount();
                if (g_top + g_line < g_count - 1 && SCROLL_NONE == g_iScrollDir)
                {
                    sndSePlay(SE_CURSOR);
                    if (g_top + g_line + LIST_LINES < g_count - LIST_LINES)
                    {
                        g_top = g_top + LIST_LINES;
                    }
                    else if (LIST_LINES < g_count)
                    {
                        g_line = LIST_LINES - 1;
                        g_top = g_count - g_line;
                    }
                    else
                    {
                        g_line = g_count - 1;
                        g_top = 0;
                    }
                    LoadList(g_top, g_line);
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                }
            }
            else
            {
                static prev_tick;
                if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_tick > 70))
                {
                    prev_tick = last_tick;
                    last_tick = tickcount();
                    new move_time = last_tick - prev_tick;
                    move_time = clamp(move_time, 4, 30);
                    if (old_line > 0 && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        MoveCursor((old_line - 1 + LIST_LINES) % LIST_LINES);
                        UpdateCounter(g_top + g_line);
                        SetBlink(1);
                        HighlightLine(listGetAt(g_line));
                    }
                    else if (g_top > 0 && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        g_top--;
                        {
                            new line = CreateLine(Self());
                            SetPos(line, list_pos[0], list_pos[1] - line_h, list_pos[2], 0);
                            listInsert(0, line);
                            SetLine(listGetAt(0), g_top);
                            sleep;
                        }
                        for (new i; i < listSize(); i++)
                        {
                            new Float:line_pos[3];
                            GetPos(listGetAt(i), line_pos[0], line_pos[1], line_pos[2], 0);
                            AnimMoveLine(listGetAt(i), line_pos[0], line_pos[1] + line_h, line_pos[2], float(move_time),
                                    0.0, ANIM_FUNC_LINEAR);
                            SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), float(move_time), 0, 0);
                        }
                        FadeAlpha(listGetLast(), 0.0, float(move_time), 0.0);
                        SendMessage(listGetLast(), TAG("FADE"), TAG("OUT"), float(move_time), 0, 0);
                        FadeAlpha(listGetAt(0), 1.0, float(move_time), 0.0);
                        SendMessage(listGetAt(0), TAG("FADE"), TAG("IN1"), float(move_time), 0, 0);
                        g_iScrollDir = SCROLL_UP;
                        g_iScrollCount++;
                        AnimTimerManager(g_objScrollTimer, float(move_time), 0.0, ANIM_FUNC_LINEAR);
                        UpdateScrollBar();
                        UpdateCounter(g_top + g_line);
                        SetBlink(1);
                    }
                }
                else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_tick > 70))
                {
                    prev_tick = last_tick;
                    last_tick = tickcount();
                    new move_time = last_tick - prev_tick;
                    move_time = clamp(move_time, 4, 30);
                    if (old_line < LIST_LINES - 1 && old_line < g_count - 1 && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        MoveCursor((old_line + 1 + LIST_LINES) % LIST_LINES);
                        UpdateCounter(g_top + g_line);
                        SetBlink(1);
                        HighlightLine(listGetAt(g_line));
                    }
                    else if (g_top < g_count - LIST_LINES && SCROLL_NONE == g_iScrollDir)
                    {
                        sndSePlay(SE_CURSOR);
                        g_top++;
                        {
                            new line = CreateLine(Self());
                            SetPos(line, list_pos[0], list_pos[1] + LIST_LINES * line_h, list_pos[2], 0);
                            listAddLast(line);
                            SetLine(listGetLast(), g_top + LIST_LINES - 1);
                            sleep;
                        }
                        for (new i; i < listSize(); i++)
                        {
                            new Float:line_pos[3];
                            GetPos(listGetAt(i), line_pos[0], line_pos[1], line_pos[2], 0);
                            AnimMoveLine(listGetAt(i), line_pos[0], line_pos[1] - line_h, line_pos[2], float(move_time),
                                    0.0, ANIM_FUNC_LINEAR);
                            SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), float(move_time), 0, 0);
                        }
                        FadeAlpha(listGetAt(0), 0.0, float(move_time), 0.0);
                        SendMessage(listGetAt(0), TAG("FADE"), TAG("OUT"), float(move_time), 0, 0);
                        FadeAlpha(listGetLast(), 1.0, float(move_time), 0.0);
                        SendMessage(listGetLast(), TAG("FADE"), TAG("IN1"), float(move_time), 0, 0);
                        g_iScrollDir = SCROLL_DOWN;
                        g_iScrollCount++;
                        AnimTimerManager(g_objScrollTimer, float(move_time), 0.0, ANIM_FUNC_LINEAR);
                        UpdateScrollBar();
                        UpdateCounter(g_top + g_line);
                        SetBlink(1);
                    }
                }
            }
        }
        FinishScroll();
        new msg;
        new what;
        new arg;
        /* The action of main ("ACT"), answered with "ACWT": "ADEL" deletes the selected entry. */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetActive(1);
                }
                else if (what == TAG("OUT"))
                {
                    SetActive(0);
                }
            }
            else if (msg == TAG("BLNK"))
            {
                if (what == TAG("IN"))
                {
                    SetBlink(1);
                    HighlightLine(listGetAt(g_line));
                }
                else if (what == TAG("OUT"))
                {
                    g_blink = 0;
                    SetBlink(0);
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
            else if (msg == TAG("ACT"))
            {
                if (what == TAG("ADEL"))
                {
                    DeleteAddr(g_top, g_line);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
            }
        }
        if (g_blink)
        {
            if (ANIM_DONE == AnimGetState(g_objCursor, ANIM_COLOR))
            {
                new Float:color[4];
                new Float:alpha = 0.7;
                if (!g_bBlinkOn)
                {
                    alpha = 0.25;
                }
                GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
                AnimColor(g_objCursor, color[0], color[1], color[2], alpha, g_flBlinkOutTime, 0.0, ANIM_FUNC_LINEAR);
                g_bBlinkOn = g_bBlinkOn ^ 1;
            }
        }
        sleep;
    }
    return;
}

/* Destroys the lines scrolled out of the list once the scroll timer is over. */
FinishScroll()
{
    if (SCROLL_NONE == g_iScrollDir)
    {
        return;
    }
    if (ANIM_DONE == AnimGetState(g_objScrollTimer, ANIM_TIMER))
    {
        if (g_iScrollDir == SCROLL_DOWN)
        {
            for (new i; i < g_iScrollCount; i++)
            {
                Destroy(listGetAt(0));
                listRemoveAt(0);
            }
        }
        else if (g_iScrollDir == SCROLL_UP)
        {
            for (new i; i < g_iScrollCount; i++)
            {
                Destroy(listGetLast());
                listRemoveLast();
            }
        }
        g_iScrollDir = SCROLL_NONE;
        g_iScrollCount = 0;
    }
}

/* Fades the title, the labels, the header and the scroll bar to alpha. */
FadeFrame(Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    for (new i; i < 3; i++)
    {
        GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objLabels[i], color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    }
    GetColor(g_objHeaderBg, color[0], color[1], color[2], color[3]);
    AnimColor(g_objHeaderBg, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
    if (g_count > LIST_LINES)
    {
        SendMessage(g_objScrollBar, TAG("FADE"), alpha, fade_time, delay, 0);
    }
    AnimTimerManager(g_objScrollTimer, fade_time, 0.0, ANIM_FUNC_LINEAR);
}

/* Fades the alpha of obj, keeping its color. */
FadeAlpha(obj, Float:alpha, Float:fade_time, Float:delay)
{
    new Float:color[4];
    GetColor(obj, color[0], color[1], color[2], color[3]);
    AnimColor(obj, color[0], color[1], color[2], alpha, fade_time, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeFrame(1.0, float(400), 0.0);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), 1.0, float(400), 0.0);
        SendMessage(listGetAt(i), TAG("FADE"), TAG("IN2"), float(400), 0, 0);
    }
}

FadeOut()
{
    FadeFrame(0.0, float(400), 0.0);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), 0.0, float(400), 0.0);
        SendMessage(listGetAt(i), TAG("FADE"), TAG("OUT"), float(400), 0, 0);
    }
}

FadeDim()
{
    FadeFrame(g_flDimAlpha, float(400), 0.0);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), g_flDimAlpha, float(400), 0.0);
        SendMessage(listGetAt(i), TAG("FADE"), TAG("DIM"), float(400), 0, 0);
    }
}

MoveCursor(line)
{
    if (g_line != line)
    {
        g_line = line;
    }
    new Float:pos[3];
    GetPos(listGetAt(g_line), pos[0], pos[1], pos[2], 0);
    SetPos(g_objCursor, pos[0], pos[1], pos[2], 0);
}

/* Shows the cursor blinking, or hides it. */
SetBlink(on)
{
    Hide(g_objCursor);
    if (on)
    {
        g_blink = 1;
        g_bBlinkOn = 0;
        if (g_count)
        {
            Show(g_objCursor);
            new Float:color[4];
            GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
            if (color[3])
            {
                SetColor(g_objCursor, color[0], color[1], color[2], 0.25);
            }
            AnimColor(g_objCursor, color[0], color[1], color[2], 0.7, g_flBlinkInTime, 0.0, ANIM_FUNC_SLOWER);
        }
    }
    else
    {
        g_blink = 0;
    }
}

/* Lights line_obj and dims the other lines, in step with a scroll in progress. */
HighlightLine(line_obj)
{
    if (g_count)
    {
        new Float:fade_time;
        if (ANIM_DONE != AnimGetState(g_objScrollTimer, ANIM_TIMER))
        {
            fade_time = float(400) * (1.0 - AnimDoTimer(g_objScrollTimer, ANIM_TIMER, 1.0));
        }
        else
        {
            fade_time = 1.0;
        }
        for (new i; i < listSize(); i++)
        {
            SendMessage(listGetAt(i), TAG("COL"), TAG("NOR"), fade_time, 0, 0);
        }
        SendMessage(line_obj, TAG("COL"), TAG("MAT"), fade_time, 0, 0);
    }
}

/* Takes the pad, with the pad help of the mode (the options only in ADDR_MODE_BOOK), or leaves it. */
SetActive(active)
{
    if (active)
    {
        if (g_count)
        {
            if (ADDR_MODE_BOOK == g_mode)
            {
                new label[16];
                get_tag_attr("TEXT", "msgmailer_btn_option", "value", label, 16);
                SetSystemParamStr(SYS_STR_PADHELP_RU, label);
                SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
            }
            else
            {
                SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
            }
        }
        else if (ADDR_MODE_BOOK == g_mode)
        {
            new label[16];
            get_tag_attr("TEXT", "msgmailer_btn_option", "value", label, 16);
            SetSystemParamStr(SYS_STR_PADHELP_RU, label);
            SetSystemParam(SYS_PADHELP, PADHELP_CROSS | PADHELP_TRIANGLE);
        }
        else
        {
            SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
        }
        g_active = 1;
    }
    else
    {
        g_active = 0;
    }
}

/* Sets the scroll bar to the visible part of the address book. */
UpdateScrollBar()
{
    if (g_count)
    {
        new Float:length = float(LIST_LINES) / float(g_count);
        length = length > 1.0 ? 1.0 : length;
        length = length < 0.0 ? 0.0 : length;
        length = length < 0.1 ? 0.1 : length;
        new Float:top = float(g_top) / float(g_count - LIST_LINES) * (1.0 - length);
        SetPubVar(g_objScrollBar, "top", top);
        if (g_count > LIST_LINES)
        {
            SendMessage(g_objScrollBar, TAG("FADE"), 1.0, float(400), 0, 0);
            SetPubVar(g_objScrollBar, "length", length);
        }
        else
        {
            SendMessage(g_objScrollBar, TAG("FADE"), 0, float(400), 0, 0);
            SetPubVar(g_objScrollBar, "length", 0);
        }
    }
    else
    {
        SetPubVar(g_objScrollBar, "top", 0);
        SetPubVar(g_objScrollBar, "length", 0);
        SendMessage(g_objScrollBar, TAG("FADE"), 0, float(400), 0, 0);
    }
}

/* Shows the position of entry index. */
UpdateCounter(index)
{
    new text[16];
    if (!g_count)
    {
        sprintf(text, "%d/%d", index, g_count);
    }
    else
    {
        sprintf(text, "%d/%d", index + 1, g_count);
    }
    SetOptionS(g_objLabels[LABEL_POSITION], TAG("UTF8"), text);
}

/* Fills the list with the address book from top, with the cursor on line. */
LoadList(top, line)
{
    g_top = top;
    g_count = mx_CountAddr();
    if (g_count - LIST_LINES < g_top)
    {
        if (--g_top < 0)
        {
            g_top = 0;
        }
    }
    if (g_count <= g_top + line)
    {
        if (--line < 0)
        {
            line = 0;
        }
    }
    for (new i; i < listSize(); i++)
    {
        SetLine(listGetAt(i), g_top + i);
    }
    MoveCursor(line);
    UpdateScrollBar();
    UpdateCounter(g_top + g_line);
}

/* Sends to line_obj the name and the address of entry index; past the end, an empty line. */
SetLine(line_obj, index)
{
    new text[32];
    new exists;
    if (index < g_count)
    {
        if (mx_CountAddr())
        {
            copy_string(text, "", 128);
            mx_GetAddrName(index, text, 128);
            SendMessageStr(line_obj, TAG("NAME"), text);
            copy_string(text, "", 128);
            mx_GetAddrAddr(index, text, 128);
            SendMessageStr(line_obj, TAG("ADDR"), text);
            exists = 1;
        }
        else
        {
            SendMessageStr(line_obj, TAG("NAME"), "");
            SendMessageStr(line_obj, TAG("ADDR"), "");
            exists = 0;
        }
    }
    else
    {
        SendMessageStr(line_obj, TAG("NAME"), "");
        SendMessageStr(line_obj, TAG("ADDR"), "");
        exists = 0;
    }
    {
        new Float:color[4];
        new Float:bright = 1.0;
        if (!exists)
        {
            bright = 0.8;
        }
        GetColor(line_obj, color[0], color[1], color[2], color[3]);
        SetColor(line_obj, bright, bright, bright, color[3]);
    }
}

/* Deletes the entry at top + line. */
DeleteAddr(top, line)
{
    mx_RemoveAddr(top + line);
    LoadList(top, line);
    for (new i; i < listSize(); i++)
    {
        FadeAlpha(listGetAt(i), 1.0, 1.0, 0.0);
    }
}

#include "mailsync.inc"
