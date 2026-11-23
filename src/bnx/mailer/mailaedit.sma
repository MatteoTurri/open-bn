/*
 * /opt0/bn/script/mailer/mailaedit.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The editor of an address book entry of the mailer: the name and the address, each edited with mailaddrskb;
 * with "mode" AEDIT_NEW a new entry, otherwise the entry g_select. The address page checks and stores it with "ACT".
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "mailer.inc"

#include "tag.inc"

#include <float>

/* The fields of the entry: the lines of g_line, g_objLabels, g_objTexts and g_szValues. */
const FIELD_NAME = 0;
const FIELD_ADDRESS = 1;

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
new Float:g_flLabelRed = 0.215;
new Float:g_flLabelGreen = 0.708;
new Float:g_flLabelBlue = 0.999;
new Float:g_flDimAlpha = 0.2;
new Float:g_flBlinkTime = 1200.0; /* ms of each step of the blink of the cursor */
new Float:g_flCursorFadeTime = 600.0;
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

public g_manager; /* the address page: holds the textures, gets "SHWT" when ready and "ACWT" after "ACT" */
public g_focus; /* unused, but part of the layout */
public g_active; /* 1 while the editor takes the pad */
public g_blink; /* the cursor blinks */
public g_select; /* the entry edited */
public g_line; /* the field of the cursor, FIELD_NAME or FIELD_ADDRESS */
public g_overwrite; /* the entry with the same address, found by "ACT" "ACHK" */
new g_objKeyboard; /* the mailaddrskb object of the edit in progress */
new g_objTitle;
new g_objCursor; /* the frame on the selected line */
new g_objGrids[2];
new g_objLabels[2];
new g_objTexts[2];
new g_szValues[2][32]; /* the name and the address */
new g_bBlinkOn;

main()
{
    new value[16];
    new Float:pos[3];
    new Float:grid_w;
    new Float:grid_h;
    new Float:line_h;
    new mode;
    if (GetArg("mode", value, 16))
    {
        mode = atoi(value);
    }
    get_group_tag_attr2("AEDIT", "HEAD", 0, "folder_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "folder_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "folder_z", value, 16);
    pos[2] = atof(value);
    get_tag_attr("TEXT", "msgmailer_addredit_title", "value", value, 16);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], 0);
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionS(g_objTitle, TAG("UTF8"), value);
    new grid_tex = GetPubVar(g_manager, "g_txAddrEditGrid");
    get_group_tag_attr2("AEDIT", "HEAD", 0, "bg_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "bg_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "bg_z", value, 16);
    pos[2] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "line_height", value, 16);
    line_h = atof(value);
    grid_w = GetPubVar(g_manager, "g_AddrEdGridWidth");
    grid_h = GetPubVar(g_manager, "g_AddrEdGridHeight");
    for (new i; i < 2; i++)
    {
        g_objGrids[i] = CreatePlane(Self());
        SetPos(g_objGrids[i], pos[0], pos[1] + line_h * i, pos[2], 0);
        SetScale(g_objGrids[i], grid_w, grid_h, 0.0);
        SetColor(g_objGrids[i], 1.0, 1.0, 1.0, 0.0);
        SetTexture(g_objGrids[i], grid_tex, 0, -1);
        SetOptionI(g_objGrids[i], TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_objGrids[i], TAG("ALPH"), BLEND_ADD);
    }
    new frame_tex = GetPubVar(g_manager, "g_txAddrEdit");
    g_objCursor = CreatePlane3x3(Self());
    SetPos(g_objCursor, 60.0 + pos[0], 0.0, 0.0, 0);
    SetColor(g_objCursor, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objCursor, frame_tex, 0, -1);
    SetOptionI(g_objCursor, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_objCursor, TAG("ZOFS"), -1.0);
    SetOptionI(g_objCursor, TAG("ALPH"), BLEND_ADD);
    grid_w = GetPubVar(g_manager, "g_AddrEditWidth");
    grid_h = GetPubVar(g_manager, "g_AddrEditHeight");
    SetScale(g_objCursor, 405.0, 32.0, 0.0);
    {
        new Float:margins[4];
        margins[0] = 5.0 / grid_w;
        margins[1] = 5.0 / grid_w;
        margins[2] = 5.0 / grid_h;
        margins[3] = 5.0 / grid_h;
        SetOptionP(g_objCursor, TAG("MGNT"), margins, 4);
        margins[0] = 5.0 / 405.0;
        margins[1] = 5.0 / 405.0;
        margins[2] = 5.0 / 32.0;
        margins[3] = 5.0 / 32.0;
        SetOptionP(g_objCursor, TAG("MGN"), margins, 4);
    }
    get_group_tag_attr2("AEDIT", "HEAD", 0, "index_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "index_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "index_z", value, 16);
    pos[2] = atof(value);
    for (new i; i < 2; i++)
    {
        if (0 == i)
        {
            get_tag_attr("TEXT", "msgmailer_addredit_item_name", "value", value, 16);
        }
        else
        {
            get_tag_attr("TEXT", "msgmailer_addredit_item_addr", "value", value, 16);
        }
        g_objLabels[i] = CreateLabel(Self());
        SetPos(g_objLabels[i], pos[0], pos[1] + line_h * i, pos[2], 0);
        SetColor(g_objLabels[i], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
        SetOptionS(g_objLabels[i], TAG("UTF8"), value);
        SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("FSIZ"), 15);
        SetOptionI(g_objLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
    }
    get_group_tag_attr2("AEDIT", "HEAD", 0, "text_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "text_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("AEDIT", "HEAD", 0, "text_z", value, 16);
    pos[2] = atof(value);
    for (new i; i < 2; i++)
    {
        g_objTexts[i] = CreateLabel(Self());
        SetPos(g_objTexts[i], pos[0], pos[1] + line_h * i, pos[2], 0);
        SetColor(g_objTexts[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionI(g_objTexts[i], TAG("HALN"), ALIGN_LEFT);
        SetOptionI(g_objTexts[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objTexts[i], TAG("FSIZ"), 15);
        SetOptionI(g_objTexts[i], TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_objTexts[i], TAG("W64 "), floatround(405.0 - 16.0, 0) * 64);
    }
    if (AEDIT_NEW != mode)
    {
        LoadEntry();
    }
    MoveCursor(FIELD_NAME);
    g_line = FIELD_NAME;
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
        if (repeated & PAD_UP && g_line == FIELD_ADDRESS)
        {
            sndSePlay(SE_CURSOR);
            g_line = FIELD_NAME;
            MoveCursor(g_line);
            SetBlink(1);
        }
        else if (repeated & PAD_DOWN && FIELD_NAME == g_line)
        {
            sndSePlay(SE_CURSOR);
            g_line = FIELD_ADDRESS;
            MoveCursor(g_line);
            SetBlink(1);
        }
        else if (pressed & PAD_CIRCLE)
        {
            EditLine(g_line);
        }
        new msg;
        new what;
        new arg;
        new extra;
        /*
         * "FOCS" IN/OUT, "BLNK" IN/OUT, "FADE" IN/OUT/DIM, and "ACT", answered with "ACWT": "ACHK" checks the entry (0
         * good, 1 empty, 2 no address, 3 the address is in entry g_overwrite), "AEDT" stores it, "ANEW" adds it, "AORW"
         * stores it over g_overwrite and removes the entry edited. Then "FNIS"/"FNIB" from mailaddrskb with the edited
         * text.
         */
        while (RecvMessage(msg, what, arg, extra))
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
                }
                else if (what == TAG("OUT"))
                {
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
                if (what == TAG("ACHK"))
                {
                    new result;
                    if (!strlen(g_szValues[FIELD_NAME]) && !strlen(g_szValues[FIELD_ADDRESS]) && mode == AEDIT_NEW)
                    {
                        result = AEDIT_CHECK_EMPTY;
                    }
                    else if (!strlen(g_szValues[FIELD_ADDRESS]))
                    {
                        result = AEDIT_CHECK_NOADDR;
                    }
                    else
                    {
                        if ((g_overwrite = mx_CheckAddr(g_select, g_szValues[FIELD_ADDRESS])) >= 0)
                        {
                            result = AEDIT_CHECK_EXISTS;
                        }
                    }
                    SendMessage(g_manager, TAG("ACWT"), result, 0, 0, 0);
                }
                else if (what == TAG("AEDT"))
                {
                    mx_SetAddr2(g_select, g_szValues[FIELD_NAME], g_szValues[FIELD_ADDRESS]);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("ANEW"))
                {
                    mx_SetAddr2(-1, g_szValues[FIELD_NAME], g_szValues[FIELD_ADDRESS]);
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
                else if (what == TAG("AORW"))
                {
                    mx_SetAddr2(g_overwrite, g_szValues[FIELD_NAME], g_szValues[FIELD_ADDRESS]);
                    if (g_select >= 0)
                    {
                        mx_RemoveAddr(g_select);
                        if (g_overwrite > 0 && g_overwrite > g_select)
                        {
                            g_overwrite--;
                        }
                    }
                    SendMessage(g_manager, TAG("ACWT"), g_overwrite, 0, 0, 0);
                }
            }
        }
        new text[32];
        while (RecvMessageStr(msg, text, 32))
        {
            if (msg == TAG("FNIS"))
            {
                FinishEdit(g_line, text, 1);
            }
            else if (msg == TAG("FNIB"))
            {
                FinishEdit(g_line, text, 0);
            }
        }
        if (g_blink)
        {
            if (ANIM_DONE == AnimGetState(g_objCursor, ANIM_COLOR))
            {
                new Float:color[4];
                new Float:alpha = 1.0;
                if (!g_bBlinkOn)
                {
                    alpha = 0.5;
                }
                GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
                AnimColor(g_objCursor, color[0], color[1], color[2], alpha, g_flBlinkTime, 0.0, ANIM_FUNC_LINEAR);
                g_bBlinkOn = g_bBlinkOn ^ 1;
            }
        }
        sleep;
    }
    return;
}

/* Fades the editor to alpha, the grids at half of it. */
FadeItems(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    for (new i; i < 2; i++)
    {
        GetColor(g_objGrids[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objGrids[i], color[0], color[1], color[2], 0.5 * alpha, duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objLabels[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objTexts[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objTexts[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeItems(1.0, float(400), 0.0);
}

FadeOut()
{
    FadeItems(0.0, float(400), 0.0);
}

FadeDim()
{
    FadeItems(g_flDimAlpha, float(400), 0.0);
}

/* Moves the cursor frame onto line. */
MoveCursor(line)
{
    new Float:grid_pos[3];
    GetPos(g_objGrids[line], grid_pos[0], grid_pos[1], grid_pos[2], 0);
    new Float:pos[3];
    GetPos(g_objCursor, pos[0], pos[1], pos[2], 0);
    SetPos(g_objCursor, pos[0], grid_pos[1], pos[2], 0);
}

/* Shows the cursor and starts its blink, or stops it. */
SetBlink(on)
{
    Hide(g_objCursor);
    if (on)
    {
        g_blink = 1;
        g_bBlinkOn = 0;
        Show(g_objCursor);
        new Float:color[4];
        GetColor(g_objCursor, color[0], color[1], color[2], color[3]);
        if (color[3])
        {
            SetColor(g_objCursor, color[0], color[1], color[2], 0.5);
        }
        AnimColor(g_objCursor, color[0], color[1], color[2], 1.0, g_flCursorFadeTime, 0.0, ANIM_FUNC_SLOWER);
    }
    else
    {
        g_blink = 0;
    }
}

SetActive(active)
{
    if (active)
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
        g_active = 1;
    }
    else
    {
        g_active = 0;
    }
}

/* Opens mailaddrskb on line, with the keyboard settings of a name or an address. */
EditLine(line)
{
    SetActive(0);
    SetBlink(0);
    new args[32];
    g_objKeyboard = CreateNull(Self());
    if (0 == line)
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;line=%d;", SKB_KEYBOARD_HIRAGANA, SKB_INPUT_DEFAULT, 0,
                SKB_LENGTH_BYTES, 128, line);
    }
    else
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;line=%d;", SKB_KEYBOARD_ALPHANUMERIC, SKB_INPUT_DIRECT,
                SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH, SKB_LENGTH_BYTES, 128, line);
    }
    AttachScript(g_objKeyboard, _, "mailaddrskb", args);
    SetPubVar(g_objKeyboard, "g_manager", Self());
    SetPubVar(g_objKeyboard, "g_manager2", g_manager);
    SetOptionI(g_objKeyboard, TAG("ZSRT"), ZSORT_OVERLAY);
    SendMessageStr(g_objKeyboard, TAG("INIT"), g_szValues[line]);
}

/* Ends the edit of line, keeping text on ok, and gives the pad back to the editor. */
FinishEdit(line, text[], ok)
{
    if (ok)
    {
        SetOptionS(g_objTexts[line], TAG("UTF8"), text);
        copy_string(g_szValues[line], text, 32);
    }
    SetColor(g_objCursor, 1.0, 1.0, 1.0, 0.5);
    Destroy(g_objKeyboard);
    SetActive(1);
    SetBlink(1);
}

/* Shows the name and the address of the entry g_select. */
LoadEntry()
{
    copy_string(g_szValues[FIELD_NAME], "", 128);
    copy_string(g_szValues[FIELD_ADDRESS], "", 128);
    if (mx_CountAddr())
    {
        new text[32];
        mx_GetAddrName(g_select, text, 128);
        SetOptionS(g_objTexts[FIELD_NAME], TAG("UTF8"), text);
        copy_string(g_szValues[FIELD_NAME], text, 32);
        mx_GetAddrAddr(g_select, text, 128);
        SetOptionS(g_objTexts[FIELD_ADDRESS], TAG("UTF8"), text);
        copy_string(g_szValues[FIELD_ADDRESS], text, 32);
    }
}

#include "mailsync.inc"
