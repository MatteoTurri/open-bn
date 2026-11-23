/*
 * /opt0/bn/script/mailer/mailaddrskb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The software keyboard of the address book editor: it edits the name (argument "line" 0) or the address
 * of an entry, with the keyboard settings in the arguments "mode", "input", "opt", "type" and "limit".
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1000

#include "tag.inc"

#include "xmlsync.inc"

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

new Float:g_flBackdropRed = 0.25;
new Float:g_flBackdropGreen = 0.25;
new Float:g_flBackdropBlue = 0.25;
new Float:g_flBackdropAlpha = 1.0;
new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flFadeTime = 300.0;
new g_szAddrForbidden[3] = {'<', '>'}; /* the characters the keyboard refuses in an address */
new g_szNameForbidden[4] = {'<', '>', ','}; /* and in a name */
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

public g_manager; /* gets "FNIS" with the edited text, or "FNIB" when the keyboard is cancelled */
public g_manager2; /* the mailer page, which holds the textures */
new g_szText[32];

main()
{
    new arg_value[32];
    new line;
    new kind;
    new input_mode;
    new flags;
    new length_unit;
    new max_length;
    if (GetArg("mode", arg_value, 32))
    {
        kind = atoi(arg_value);
    }
    if (GetArg("input", arg_value, 32))
    {
        input_mode = atoi(arg_value);
    }
    if (GetArg("opt", arg_value, 32))
    {
        flags = atoi(arg_value);
    }
    if (GetArg("type", arg_value, 32))
    {
        length_unit = atoi(arg_value);
    }
    if (GetArg("limit", arg_value, 32))
    {
        max_length = atoi(arg_value);
    }
    if (GetArg("line", arg_value, 32))
    {
        line = atoi(arg_value);
    }
    new ok = EditOnInit(kind, input_mode, flags, length_unit, max_length, line);
    if (ok)
    {
        skbGetString(g_szText, 32);
        SendMessageStr(g_manager, TAG("FNIS"), g_szText);
    }
    else
    {
        SendMessageStr(g_manager, TAG("FNIB"), g_szText);
    }
    return;
}

/* Waits for "INIT" with the text to edit, then edits it; returns 1 if the keyboard was closed with OK. */
EditOnInit(kind, input_mode, flags, length_unit, max_length, line)
{
    new ok;
    for (; ; )
    {
        new msg;
        while (RecvMessageStr(msg, g_szText, 32))
        {
            if (msg == TAG("INIT"))
            {
                ok = RunKeyboard(g_szText, kind, input_mode, flags, length_unit, max_length, line);
                return ok;
            }
        }
        sleep;
    }
    return ok;
}

/* Edits text with the software keyboard over the address editor; returns 1 and the new text on OK. */
RunKeyboard(text[], kind, input_mode, flags, length_unit, max_length, line)
{
    new value[16];
    new Float:pos[3];
    new Float:grid_w;
    new Float:grid_h;
    new backdrop = CreateDefocus(Self());
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, g_flBackdropAlpha, g_flFadeTime, 0.0,
            ANIM_FUNC_LINEAR);
    new grid_tex = GetPubVar(g_manager2, "g_txAddrEditGrid");
    get_group_tag_attr("AEDIT", "HEAD", 0, "editbg_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr("AEDIT", "HEAD", 0, "editbg_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr("AEDIT", "HEAD", 0, "editbg_z", value, 16);
    pos[2] = atof(value);
    grid_w = GetPubVar(g_manager2, "g_AddrEdGridWidth");
    grid_h = GetPubVar(g_manager2, "g_AddrEdGridHeight");
    new grid = CreatePlane(backdrop);
    SetPos(grid, pos[0], pos[1], pos[2], 0);
    SetScale(grid, grid_w, grid_h, 0.0);
    SetTexture(grid, grid_tex, 0, -1);
    SetColor(grid, 1.0, 1.0, 1.0, 0.0);
    AnimColor(grid, 1.0, 1.0, 1.0, 1.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    get_group_tag_attr("AEDIT", "HEAD", 0, "editindex_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr("AEDIT", "HEAD", 0, "editindex_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr("AEDIT", "HEAD", 0, "editindex_z", value, 16);
    pos[2] = atof(value);
    new caption = CreateLabel(backdrop);
    SetPos(caption, pos[0], 1.0 + pos[1], pos[2], 0);
    SetOptionI(caption, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(caption, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(caption, TAG("FSIZ"), 15);
    if (0 == line)
    {
        get_tag_attr("TEXT", "msgmailer_addredit_item_name", "value", value, 16);
    }
    else
    {
        get_tag_attr("TEXT", "msgmailer_addredit_item_addr", "value", value, 16);
    }
    SetOptionS(caption, TAG("UTF8"), value);
    SetColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    AnimColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    get_group_tag_attr("AEDIT", "HEAD", 0, "edittext_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr("AEDIT", "HEAD", 0, "edittext_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr("AEDIT", "HEAD", 0, "edittext_z", value, 16);
    pos[2] = atof(value);
    new edit = CreateEdit(backdrop, 1024);
    SetPos(edit, pos[0], pos[1] - 8.0, pos[2], 0);
    SetColor(edit, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0);
    SetOptionF(edit, TAG("ZOFS"), 2.0);
    SetOptionI(edit, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(edit, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(edit, TAG("FSIZ"), 15);
    SetOptionS(edit, TAG("UTF8"), text);
    SetOptionI(edit, TAG("W64 "), 380 * 64);
    SetOptionI(edit, TAG("H64 "), 20 * 64);
    SetOptionI(edit, TAG("LM64"), 60 * 64);
    SetOptionI(edit, TAG("RM64"), 60 * 64);
    {
        new bg_colors[4];
        static bg_bytes[16];
        pack_color_bytes(bg_colors, bg_bytes);
        SetOptionP(edit, TAG("BGC4"), bg_colors, 4);
    }
    sndSePlay(SE_KEYBOARD_OPEN);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    new skb;
    if (0 == line)
    {
        skb = skbStart(edit, kind, input_mode, flags, length_unit, max_length, g_szNameForbidden, 0);
    }
    else
    {
        skb = skbStart(edit, kind, input_mode, flags, length_unit, max_length, g_szAddrForbidden, 0);
    }
    if (skb == -1)
    {
        return 0;
    }
    new ok;
    if (wait_skb() == 1)
    {
        skbGetString(text, 128); /* NOTE: bug: maxlength counts cells, and text (g_szText) has 32 */
        ok = 1;
    }
    new Float:color[4];
    GetColor(grid, color[0], color[1], color[2], color[3]);
    AnimColor(grid, color[0], color[1], color[2], 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    GetColor(caption, color[0], color[1], color[2], color[3]);
    AnimColor(caption, color[0], color[1], color[2], 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    Destroy(edit);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    Destroy(backdrop);
    return ok;
}
