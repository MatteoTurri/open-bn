/*
 * /opt0/bn/script/mailer/mailseditdetail.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The detailed settings of the mailer (authentication, keeping mail on the server, ports, POP before
 * SMTP, SMTP authentication with its user and password) as a list under a title (argument "top"),
 * and a button that restores the defaults. Choices are made by the settings page, which reattaches the
 * script with "edit" (the item) and "parts" (the choice); the texts are edited here with mailsetskb.
 */

#include <core>
#include <bn>

#pragma dynamic 3000

#include "mailer.inc"

#include "tag.inc"

#include <float>

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
new Float:g_flDisabledAlpha = 0.35;
new g_iItemCount;
new g_szAnyId[1]; /* an empty id: the first element with the tag */
new g_szItemId[64];

/* Fills ids with the ids of the empty leaves of the first tag element; returns their number. */
GetItemIds(tag[], ids[][])
{
    g_iItemCount = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    CollectItemIds(node, ids);
    return g_iItemCount;
}

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

/* Returns the index of item id among the empty leaves of the first tag element, 0 when it is not there. */
GetItemIndex(tag[], id[])
{
    g_iItemCount = 0;
    g_iItemIndex = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    FindItemIndex(node, id);
    return g_iItemIndex;
}

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

#include "focus.inc"

public g_manager; /* the settings page: gets "SHWT" when the list is ready and "ACWT" after "ACT" */
public g_focus; /* the focus object, moved onto the selected item */
public g_active; /* 1 while the list has the focus and takes the pad */
public g_iSetMenu; /* the selected item */
public g_chkskb; /* 1 while the selected item takes the circle button here (the texts and the default button) */
new g_objTitle;
new g_objLabels[9];
new g_objValues[9];
new g_objKeyboard; /* the mailsetskb object of the edit in progress */
new g_objButton; /* the button of the default item, with its window and glow below */
new g_objButtonBase;
new g_objButtonWindow;
new g_objButtonGlow;
new g_objFadeTimer; /* its timer animation lasts as long as the last fade of the list */
new g_szItemIds[9][16]; /* the ids of the items, from SEDIT_DETAIL_ITEM */
new g_iChoices[9]; /* the value of each item: a choice or a port number */
new g_szAuthUser[32];
new g_szAuthPass[30];
new g_bEnabled[9]; /* the items that can be selected */
new g_iDefaultItem; /* the item shown as the button that restores the defaults */

#include "pfs.inc"

/* Like fit_button_to_label, for the label of the default item. */
FitDefaultButtonToLabel(&Float:button_w, &Float:button_h)
{
    for (new rect[5]; ; )
    {
        GetOptionP(g_objLabels[g_iDefaultItem], TAG("RECT"), rect, 5);
        if (rect[0])
        {
            new Float:extent = float(rect[3] >> 6) + float(30);
            if (button_w < extent)
            {
                button_w = extent;
            }
            extent = 20.0 + float(rect[4] >> 6);
            if (button_h < extent)
            {
                button_h = extent;
            }
            break;
        }
        sleep;
    }
}

main()
{
    new Float:label_pos[3];
    new Float:value_pos[3];
    new Float:line_h;
    new last_item;
    new text[32];
    g_iDefaultItem = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_DEFAULT");
    copy_string(g_szAuthUser, "", 128);
    copy_string(g_szAuthPass, "", 120);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "folder_x", text, 32);
    label_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "folder_y", text, 32);
    label_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "folder_z", text, 32);
    label_pos[2] = atof(text);
    GetArg("top", text, 32);
    g_objTitle = CreateLabel(Self());
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetPos(g_objTitle, label_pos[0], label_pos[1], label_pos[2], 0);
    SetOptionS(g_objTitle, TAG("UTF8"), text);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    GetItemIds("SEDIT_DETAIL_ITEM", g_szItemIds);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "item_x", text, 32);
    label_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "item_y", text, 32);
    label_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "item_z", text, 32);
    label_pos[2] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "info_x", text, 32);
    value_pos[0] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "info_y", text, 32);
    value_pos[1] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "info_z", text, 32);
    value_pos[2] = atof(text);
    get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "line_height", text, 32);
    line_h = atof(text);
    for (new i; i < 9; i++)
    {
        g_bEnabled[i] = 1;
        last_item = i;
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_CERTIFY"))
        {
            if (GetArg("edit", text, 32) && i == atoi(text))
            {
                GetArg("parts", text, 32);
                g_iChoices[i] = atoi(text);
                mx_SetPref("CERTIFY", g_iChoices[i]);
                mx_Restart();
            }
            else
            {
                mx_GetPref("CERTIFY", g_iChoices[i]);
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_MAIL"))
        {
            if (GetArg("edit", text, 32) && i == atoi(text))
            {
                GetArg("parts", text, 32);
                g_iChoices[i] = atoi(text);
                mx_SetPref("RMMAIL", g_iChoices[i]);
                mx_Restart();
            }
            else
            {
                mx_GetPref("RMMAIL", g_iChoices[i]);
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTP"))
        {
            if (GetArg("edit", text, 32) && i == atoi(text))
            {
                GetArg("parts", text, 32);
                g_iChoices[i] = atoi(text);
                mx_SetPref("SMTP_PORT", g_iChoices[i]);
                mx_Restart();
            }
            else
            {
                mx_GetPref("SMTP_PORT", g_iChoices[i]);
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_POP3"))
        {
            if (GetArg("edit", text, 32) && i == atoi(text))
            {
                GetArg("parts", text, 32);
                g_iChoices[i] = atoi(text);
                mx_SetPref("POP3_PORT", g_iChoices[i]);
                mx_Restart();
            }
            else
            {
                mx_GetPref("POP3_PORT", g_iChoices[i]);
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_POPSMTP"))
        {
            if (GetArg("edit", text, 32) && i == atoi(text))
            {
                GetArg("parts", text, 32);
                g_iChoices[i] = atoi(text);
                mx_SetPref("POPSMTP", g_iChoices[i] ^ 1);
                mx_Restart();
            }
            else
            {
                mx_GetPref("POPSMTP", g_iChoices[i]);
                g_iChoices[i] ^= 1;
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTPAUTH"))
        {
            if (GetArg("edit", text, 32) && i == atoi(text))
            {
                GetArg("parts", text, 32);
                g_iChoices[i] = atoi(text);
                mx_SetPref("SMTPAUTH", g_iChoices[i] ^ 1);
                mx_Restart();
            }
            else
            {
                mx_GetPref("SMTPAUTH", g_iChoices[i]);
                g_iChoices[i] ^= 1;
            }
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER"))
        {
            mx_GetPrefStr("SMTPAUTH_USER", g_szAuthUser, 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
        {
            mx_GetPrefStr("SMTPAUTH_PASS", g_szAuthPass, 120);
        }
        new label[64];
        new value[64];
        GetItemTexts(i, label, value);
        g_objLabels[i] = CreateLabel(Self());
        SetPos(g_objLabels[i], label_pos[0], label_pos[1] + i * line_h, label_pos[2], 0);
        SetColor(g_objLabels[i], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
        SetOptionS(g_objLabels[i], TAG("UTF8"), label);
        SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_objLabels[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objLabels[i], TAG("FSIZ"), 15);
        SetOptionI(g_objLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
        g_objValues[i] = CreateLabel(Self());
        SetPos(g_objValues[i], value_pos[0], value_pos[1] + i * line_h, value_pos[2], 0);
        SetColor(g_objValues[i], g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionS(g_objValues[i], TAG("UTF8"), value);
        SetOptionI(g_objValues[i], TAG("HALN"), ALIGN_LEFT);
        SetOptionI(g_objValues[i], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_objValues[i], TAG("FSIZ"), 15);
        SetOptionI(g_objValues[i], TAG("W64 "), 200 * 64);
        SetOptionI(g_objValues[i], TAG("ZSRT"), ZSORT_OVERLAY);
        if (i == g_iDefaultItem)
        {
            SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_CENTER);
            GetPos(g_objLabels[i], label_pos[0], label_pos[1], label_pos[2], 0);
            get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "def_x", text, 32);
            label_pos[0] = atof(text);
            get_group_tag_attr2("SEDIT_DETAIL", "HEAD", 0, "def_y", text, 32);
            label_pos[1] = atof(text);
            SetPos(g_objLabels[i], label_pos[0], label_pos[1], label_pos[2], 0);
        }
    }
    GetPos(g_objLabels[g_iDefaultItem], label_pos[0], label_pos[1], label_pos[2], 0);
    g_objButton = create_button_white(Self(), 0.0, 78.0);
    SetTexture(g_objButton, imgGetResource(RES_BUTTON0), 0, -1);
    SetPos(g_objButton, label_pos[0], label_pos[1], label_pos[2], 0);
    g_objButtonBase = CreatePlane(Self());
    SetPos(g_objButtonBase, label_pos[0], label_pos[1], label_pos[2], 0);
    SetColor(g_objButtonBase, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objButtonBase, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_objButtonBase, TAG("ZOFS"), -2.0);
    SetOptionI(g_objButtonBase, TAG("ALPH"), BLEND_KEEP);
    g_objButtonWindow = CreatePlane3x3(g_objButtonBase);
    SetColor(g_objButtonWindow, 0.0, 0.2353, 0.3922, 0.0);
    SetTexture(g_objButtonWindow, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_objButtonWindow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_objButtonWindow, TAG("ZOFS"), -1.0);
    SetOptionI(g_objButtonWindow, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_objButtonWindow, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / 400.0;
        margins[2] = margins[3] = 0.5 * 128.0 / 260.0;
        SetOptionP(g_objButtonWindow, TAG("MGN"), margins, 4);
        g_objButtonGlow = CreatePlane(g_objButtonWindow);
        SetPos(g_objButtonGlow, 0.3, -0.2, 0.0, 0);
        SetScale(g_objButtonGlow, 2.0, 2.0, 1.0);
        SetColor(g_objButtonGlow, 0.0, 0.0, 0.0, 0.0);
        SetTexture(g_objButtonGlow, imgGetResource(RES_DOT), 0, -1);
        SetOptionI(g_objButtonGlow, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(g_objButtonGlow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    }
    {
        new Float:button_w;
        new Float:button_h;
        FitDefaultButtonToLabel(button_w, button_h);
        SetScale(g_objButton, button_w, button_h, 1.0);
        SetLocalScale(g_objButtonBase, 2.0 * button_w, 2.0 * button_h, 1.0);
        SetScale(g_objButtonWindow, button_w - 5.0, button_h - 5.0, 1.0);
    }
    g_objFadeTimer = CreateNull(Self());
    UpdateAuthItems();
    SelectItem(g_iSetMenu);
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
        /* refocus: the focus must move to the selected item; last_repeat: time of the last auto-repeat step */
        static refocus;
        static last_repeat;
        if (pressed & PAD_UP || (repeated & PAD_UP && tickcount() - last_repeat > 70))
        {
            last_repeat = tickcount();
            new item = g_iSetMenu;
            while (item > 0)
            {
                if (g_bEnabled[--item])
                {
                    g_iSetMenu = item;
                    SelectItem(g_iSetMenu);
                    sndSePlay(SE_CURSOR);
                    refocus = 1;
                    break;
                }
            }
        }
        else if (pressed & PAD_DOWN || (repeated & PAD_DOWN && tickcount() - last_repeat > 70))
        {
            last_repeat = tickcount();
            new item = g_iSetMenu;
            while (item < last_item)
            {
                if (g_bEnabled[++item])
                {
                    g_iSetMenu = item;
                    SelectItem(g_iSetMenu);
                    sndSePlay(SE_CURSOR);
                    refocus = 1;
                    break;
                }
            }
        }
        else if (pressed & PAD_CIRCLE)
        {
            if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER"))
            {
                new title[16];
                get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth_user", "value", title, 16);
                EditText(g_szAuthUser, 0, title);
            }
            else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
            {
                new title[16];
                get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth_pass", "value", title, 16);
                EditText(g_szAuthPass, 1, title);
            }
            else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_DEFAULT"))
            {
                SendMessage(g_manager, TAG("DLG"), TAG("DDEF"), 0, 0, 0);
            }
        }
        if (g_active && refocus)
        {
            if (g_iSetMenu == g_iDefaultItem)
            {
                send_focus(g_focus, g_objButton, 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            }
            else
            {
                send_label_focus2(g_focus, g_objLabels[g_iSetMenu], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            }
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /*
         * "FOCS" IN/OUT, "FADE" IN/OUT/DIM, "ACT" "PART" (answered with "ACWT" and the arguments of the choices of the
         * item) and "ACT" "DEFT" (restore the defaults, then "ACWT"); then "FNIS"/"FNIB" from mailsetskb with the
         * edited text.
         */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    SetActive(1);
                    refocus = 1;
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
            else if (msg == TAG("ACT"))
            {
                if (what == TAG("PART"))
                {
                    new item;
                    if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_CERTIFY"))
                    {
                        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_CERTIFY");
                        GetChoiceArgs(text, item);
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_MAIL"))
                    {
                        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_MAIL");
                        GetChoiceArgs(text, item);
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTP"))
                    {
                        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTP");
                        GetSpinArgs(text, item, 65535, 0, 4, "");
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_POP3"))
                    {
                        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_POP3");
                        GetSpinArgs(text, item, 65535, 0, 4, "");
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_POPSMTP"))
                    {
                        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_POPSMTP");
                        GetChoiceArgs(text, item);
                    }
                    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH"))
                    {
                        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTPAUTH");
                        GetChoiceArgs(text, item);
                    }
                    SendMessageStr(g_manager, TAG("ACWT"), text);
                }
                else if (what == TAG("DEFT"))
                {
                    RestoreDefaults();
                    SendMessage(g_manager, TAG("ACWT"), 0, 0, 0, 0);
                }
            }
        }
        while (RecvMessageStr(msg, text, 32))
        {
            if (msg == TAG("FNIS"))
            {
                FinishEdit(text, 1);
                refocus = 1;
            }
            else if (msg == TAG("FNIB"))
            {
                FinishEdit(text, 0);
                refocus = 1;
            }
        }
        sleep;
    }
    return;
}

/* Fades every label of the list and the button to alpha, the disabled items dimmer. */
FadeItems(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    for (new i; i < 9; i++)
    {
        new Float:item_alpha = alpha;
        if (!g_bEnabled[i])
        {
            item_alpha = item_alpha * g_flDisabledAlpha;
        }
        GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objLabels[i], color[0], color[1], color[2], item_alpha, duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objValues[i], color[0], color[1], color[2], color[3]);
        AnimColor(g_objValues[i], color[0], color[1], color[2], item_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButton, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButton, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButtonBase, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButtonBase, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButtonWindow, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButtonWindow, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objButtonGlow, color[0], color[1], color[2], color[3]);
    AnimColor(g_objButtonGlow, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimTimerManager(g_objFadeTimer, duration, 0.0, ANIM_FUNC_LINEAR);
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

/* Gives the list the pad and the pad help, or takes them away with the focus. */
SetActive(active)
{
    if (active)
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
        g_active = 1;
    }
    else
    {
        send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        g_active = 0;
    }
}

/* Updates g_chkskb for the selected item and lights the label of the default button while it is selected. */
SelectItem(index)
{
    new Float:color[4];
    if (index == g_iDefaultItem)
    {
        g_chkskb = 1;
        if (ANIM_DONE != AnimGetState(g_objFadeTimer, ANIM_TIMER))
        {
            new Float:fade_time = float(400) * (1.0 - AnimDoTimer(g_objFadeTimer, ANIM_TIMER, 1.0));
            AnimColor(g_objLabels[g_iDefaultItem], g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, fade_time, 0.0,
                    ANIM_FUNC_LINEAR);
        }
        else
        {
            GetColor(g_objLabels[g_iDefaultItem], color[0], color[1], color[2], color[3]);
            SetColor(g_objLabels[g_iDefaultItem], g_flTextRed, g_flTextGreen, g_flTextBlue, color[3]);
        }
    }
    else
    {
        if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER")
                || !strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
        {
            g_chkskb = 1;
        }
        else
        {
            g_chkskb = 0;
        }
        if (ANIM_DONE != AnimGetState(g_objFadeTimer, ANIM_TIMER))
        {
            new Float:fade_time = float(400) * (1.0 - AnimDoTimer(g_objFadeTimer, ANIM_TIMER, 1.0));
            AnimColor(g_objLabels[g_iDefaultItem], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 1.0, fade_time, 0.0,
                    ANIM_FUNC_LINEAR);
        }
        else
        {
            GetColor(g_objLabels[g_iDefaultItem], color[0], color[1], color[2], color[3]);
            SetColor(g_objLabels[g_iDefaultItem], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, color[3]);
        }
    }
}

/* Enables the user and password items only with SMTP authentication on. */
UpdateAuthItems()
{
    new item;
    item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTPAUTH");
    new enabled = g_iChoices[item] ^ 1;
    item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTPAUTH_USER");
    g_bEnabled[item] = enabled;
    item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS");
    g_bEnabled[item] = enabled;
}

/* Gets the caption of item index and the text of its current value. */
GetItemTexts(index, label[], value[])
{
    new label_text[64];
    new value_text[64];
    new item;
    new yes_no_ids[2][16];
    GetItemIds("SEDIT_COMMON_ITEM", yes_no_ids);
    if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_CERTIFY"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_cert", "value", label_text, 64);
        new certify_ids[2][16];
        GetItemIds("SEDIT_DETAIL_CERTIFY_ITEM", certify_ids);
        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_CERTIFY");
        if (!strcmp(certify_ids[g_iChoices[item]], "MAILER_SEDIT_DETAIL_NORMAL"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_cert_std", "value", value_text, 64);
        }
        else if (!strcmp(certify_ids[g_iChoices[item]], "MAILER_SEDIT_DETAIL_APOP"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_cert_apop", "value", value_text, 64);
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_MAIL"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_keep", "value", label_text, 64);
        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_MAIL");
        if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_YES"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_keep_yes", "value", value_text, 64);
        }
        else if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_NO"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_keep_no", "value", value_text, 64);
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_SMTP"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpport", "value", label_text, 64);
        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTP");
        sprintf(value_text, "%d", g_iChoices[item]);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_POP3"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_pop3port", "value", label_text, 64);
        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_POP3");
        sprintf(value_text, "%d", g_iChoices[item]);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_POPSMTP"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_popsmtp", "value", label_text, 64);
        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_POPSMTP");
        if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_YES"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_popsmtp_yes", "value", value_text, 64);
        }
        else if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_NO"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_popsmtp_no", "value", value_text, 64);
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_SMTPAUTH"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth", "value", label_text, 64);
        item = GetItemIndex("SEDIT_DETAIL_ITEM", "MAILER_SEDIT_DETAIL_SMTPAUTH");
        if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_YES"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth_yes", "value", value_text, 64);
        }
        else if (!strcmp(yes_no_ids[g_iChoices[item]], "MAILER_SEDIT_NO"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth_no", "value", value_text, 64);
        }
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth_user", "value", label_text, 64);
        copy_string(value_text, g_szAuthUser, 128);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_smtpauth_pass", "value", label_text, 64);
        mask_password(value_text, g_szAuthPass);
    }
    else if (!strcmp(g_szItemIds[index], "MAILER_SEDIT_DETAIL_DEFAULT"))
    {
        get_tag_attr("TEXT", "msgmailer_set_item_detail_default", "value", label_text, 64);
        copy_string(value_text, "", 256);
    }
    copy_string(label, label_text, 256);
    copy_string(value, value_text, 256);
}

/* Writes in args the arguments of the two choices of item. */
GetChoiceArgs(args[], item)
{
    new label[32];
    new choices[10][32];
    {
        new saved = g_iChoices[item];
        g_iChoices[item] = 0;
        for (new i; i < 2; i++)
        {
            GetItemTexts(item, label, choices[i]);
            g_iChoices[item]++;
        }
        g_iChoices[item] = saved;
    }
    sprintf(args, "btn0=%s;btn1=%s;label=%s;sel=%d;", choices[0], choices[1], label, g_iChoices[item]);
}

/* Writes in args the arguments of the spin of the number of item. */
GetSpinArgs(args[], item, max_value, min_value, speed, suffix[])
{
    new label[64];
    new value[64];
    GetItemTexts(item, label, value);
    sprintf(args, "max=%d;min=%d;spd=%d;label=%s;end=%s;sel=%d;", max_value, min_value, speed, label, suffix,
            g_iChoices[item]);
}

/* Opens mailsetskb on the selected text item. */
EditText(text[], password, title[])
{
    SetActive(0);
    new args[64];
    g_objKeyboard = CreateNull(Self());
    if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_QWERTY,
                SKB_INPUT_DIRECT, SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_HALF_WIDTH,
                SKB_LENGTH_BYTES, 128, title, password, 1);
    }
    else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
    {
        sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;pass=%d;ucs4=%d;", SKB_KEYBOARD_QWERTY,
                SKB_INPUT_DIRECT, SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_HALF_WIDTH,
                SKB_LENGTH_BYTES, 120, title, password, 1);
    }
    AttachScript(g_objKeyboard, _, "mailsetskb", args);
    SetPubVar(g_objKeyboard, "g_manager", Self());
    SetOptionI(g_objKeyboard, TAG("ZSRT"), ZSORT_OVERLAY);
    SendMessageStr(g_objKeyboard, TAG("INIT"), text);
}

/* Ends the edit of the selected text item: on ok stores text in the preferences and restarts the mailer. */
FinishEdit(text[], ok)
{
    if (ok)
    {
        new shown[32];
        if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER"))
        {
            copy_string(g_szAuthUser, text, 32);
            copy_string(shown, text, 32);
            mx_SetPrefStr("SMTPAUTH_USER", g_szAuthUser);
            mx_Restart();
        }
        else if (!strcmp(g_szItemIds[g_iSetMenu], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
        {
            copy_string(g_szAuthPass, text, 30);
            mask_password(shown, text);
            mx_SetPrefStr("SMTPAUTH_PASS", g_szAuthPass);
            mx_Restart();
        }
        SetOptionS(g_objValues[g_iSetMenu], TAG("UTF8"), shown);
    }
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    Destroy(g_objKeyboard);
    SetActive(1);
}

/* Restores the default preferences and shows them. */
RestoreDefaults()
{
    mx_DefaultPref();
    mx_Restart();
    for (new i; i < 9; i++)
    {
        g_bEnabled[i] = 1;
        if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_CERTIFY"))
        {
            mx_GetPref("CERTIFY", g_iChoices[i]);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_MAIL"))
        {
            mx_GetPref("RMMAIL", g_iChoices[i]);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTP"))
        {
            mx_GetPref("SMTP_PORT", g_iChoices[i]);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_POP3"))
        {
            mx_GetPref("POP3_PORT", g_iChoices[i]);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_POPSMTP"))
        {
            mx_GetPref("POPSMTP", g_iChoices[i]);
            g_iChoices[i] ^= 1;
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTPAUTH"))
        {
            mx_GetPref("SMTPAUTH", g_iChoices[i]);
            g_iChoices[i] ^= 1;
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTPAUTH_USER"))
        {
            mx_GetPrefStr("SMTPAUTH_USER", g_szAuthUser, 128);
        }
        else if (!strcmp(g_szItemIds[i], "MAILER_SEDIT_DETAIL_SMTPAUTH_PASS"))
        {
            mx_GetPrefStr("SMTPAUTH_PASS", g_szAuthPass, 120);
        }
        new label[64];
        new value[64];
        GetItemTexts(i, label, value);
        SetOptionS(g_objLabels[i], TAG("UTF8"), label);
        SetOptionS(g_objValues[i], TAG("UTF8"), value);
    }
    UpdateAuthItems();
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
}

#include "mailsync.inc"
