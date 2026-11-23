/*
 * /opt0/bn/script/system/fg.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The foreground window: the bars at the top and the bottom of the screen with the pad help, the L1 and R1
 * marks, the page number, the busy indicator and the mail and secure page icons. It loads the common
 * background first.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "wait.inc"

#include "tag.inc"

#include <float>

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

new g_common_bg_name[] = "SCEI:CommonBG";

/* Loads the page of "LINK" link_id as the common background, unless the background window already shows it. */
load_common_bg(link_id[], Float:fade_time)
{
    {
        new name[256];
        if (wndGetName(WND_BG, name, 256) && 0 == strcmp(name, g_common_bg_name))
        {
            return;
        }
    }
    new url[256];
    get_tag_attr("LINK", link_id, "href", url, 256);
    xmlLoadStart(url, g_common_bg_name, "", XMLLOAD_BG);
    wait_xml_load();
    fade_in_background(fade_time);
}

#include "background.inc"

/* The buttons of the pad help, the index of g_button_*: the first four at the bottom, L1 and R1 at the top. */
const BUTTON_SQUARE = 0;
const BUTTON_CROSS = 1;
const BUTTON_CIRCLE = 2;
const BUTTON_TRIANGLE = 3;
const BUTTON_L1 = 4;
const BUTTON_R1 = 5;

new g_bars[2];
new g_bar_lines[2];
new g_button_icons[6];
new g_button_labels[6];
new g_logo;
new g_busy;
new g_mail_icon;
new g_lock_icon;
new g_page_label;
new g_pad_help;
new g_last_pad_help;
new g_page_max;
new g_page_no;
new g_button_textures[6] = {RES_BTN_L, RES_BTN_D, RES_BTN_R, RES_BTN_U, RES_BTN_L1, RES_BTN_R1};
new g_button_texts[6][8];
new g_button_text_ids[6][8] = {"fg:btn_l", "msgtop_btn_back", "msgtop_btn_enter", "msgtop_btn_option", "", ""};
new g_button_bits[6] = {PADHELP_SQUARE, PADHELP_CROSS, PADHELP_CIRCLE, PADHELP_TRIANGLE, PADHELP_L1, PADHELP_R1};
new g_label_params[4] = {SYS_STR_PADHELP_RL, SYS_STR_PADHELP_RD, SYS_STR_PADHELP_RR, SYS_STR_PADHELP_RU};
new g_default_params[4] = {SYS_STR_PADHELP_DEF_RL, SYS_STR_PADHELP_DEF_RD, SYS_STR_PADHELP_DEF_RR,
        SYS_STR_PADHELP_DEF_RU};
new g_button_shown[6];
new g_mail_shown;
new g_lock_shown;
new Float:g_button_x[8][3] = {
    {24.0, 236.0, 339.0}, {24.0, 213.0, 335.0}, {24.0, 188.0, 302.0}, {24.0, 197.0, 343.0},
    {24.0, 193.0, 333.0}, {24.0, 233.0, 350.0}, {24.0, 193.0, 362.0}, {24.0, 164.0, 313.0}
};
new Float:g_lr_x[2] = {260.0, 380.0};
new g_label_ready[6]; /* the label has been laid out */
new Float:g_label_w[6];
new Float:g_icon_size = 24.0;
new Float:g_lr_w = 55.0;
new Float:g_lr_h = 36.0;
new Float:g_bar_alpha = 1.0;
new Float:g_line_alpha = 0.28;
new Float:g_logo_y = 9.0;
new Float:g_logo_w = 175.0;
new Float:g_logo_h = 32.0;
new Float:g_fade_time = 300.0;
new Float:g_screen_w = 640.0;
new Float:g_screen_h = 480.0;
new Float:g_center_x = 320.0;

is_lr_button(index)
{
    return index == BUTTON_L1 || index == BUTTON_R1;
}

create_pad_button(index)
{
    if (!is_lr_button(index))
    {
        g_button_icons[index] = CreatePlane(g_bars[1]);
        SetLocalScale(g_button_icons[index], g_icon_size, g_icon_size, 1.0);
        SetColor(g_button_icons[index], 1.0, 1.0, 1.0, 0.0);
        SetTexture(g_button_icons[index], imgGetResource(g_button_textures[index]), 0, -1);
        SetOptionI(g_button_icons[index], TAG("ALPH"), BLEND_ADD);
        Hide(g_button_icons[index]);
        g_button_labels[index] = CreateLabel(g_bars[1]);
        SetColor(g_button_labels[index], 1.0, 1.0, 1.0, 0.0);
        SetOptionI(g_button_labels[index], TAG("FSIZ"), 14);
        SetOptionI(g_button_labels[index], TAG("ALPH"), BLEND_ADD);
        SetOptionI(g_button_labels[index], TAG("VALN"), ALIGN_CENTER);
        SetOptionS(g_button_labels[index], TAG("UTF8"), g_button_texts[index]);
    }
    else
    {
        g_button_icons[index] = CreatePlane(g_bars[0]);
        SetLocalScale(g_button_icons[index], g_lr_w, g_lr_h, 1.0);
        SetColor(g_button_icons[index], 1.0, 1.0, 1.0, 0.0);
        SetTexture(g_button_icons[index], imgGetResource(g_button_textures[index]), 0, -1);
        SetOptionI(g_button_icons[index], TAG("ALPH"), BLEND_ADD);
        Hide(g_button_icons[index]);
    }
}

/* Replaces the label of a pad help button, fading out the old one. */
set_button_text(index, text[])
{
    if (!is_lr_button(index))
    {
        sprintf(g_button_texts[index], text);
        AnimColor(g_button_icons[index], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_button_labels[index], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        TimerDie(g_button_icons[index], ms_to_frames(200));
        TimerDie(g_button_labels[index], ms_to_frames(200));
        g_button_icons[index] = 0;
        g_button_labels[index] = 0;
        create_pad_button(index);
        g_label_w[index] = 0.0;
        g_label_ready[index] = 0;
    }
}

main()
{
    {
        new Float:bar_h = 64.0;
        new Float:half_h = bar_h / 2.0;
        new Float:h = bar_h;
        g_bars[0] = CreatePlane(Disp2D(0));
        SetPos(g_bars[0], g_center_x, half_h, 0.0, 0);
        SetLocalScale(g_bars[0], g_screen_w, h, 1.0);
        SetColor(g_bars[0], 0.0, 0.0, 0.0, 0.0);
        SetOptionF(g_bars[0], TAG("ZOFS"), -2.0);
        g_bars[1] = CreatePlane(Disp2D(0));
        SetPos(g_bars[1], g_center_x, g_screen_h - half_h, 0.0, 0);
        SetLocalScale(g_bars[1], g_screen_w, h, 1.0);
        SetColor(g_bars[1], 0.0, 0.0, 0.0, 0.0);
        SetOptionF(g_bars[1], TAG("ZOFS"), -2.0);
        g_bar_lines[0] = CreatePlane(Disp2D(0));
        SetPos(g_bar_lines[0], g_center_x, bar_h - 0.5, 0.0, 0);
        SetLocalScale(g_bar_lines[0], g_screen_w, 1.0, 1.0);
        SetColor(g_bar_lines[0], 1.0, 1.0, 1.0, 0.0);
        SetOptionI(g_bar_lines[0], TAG("ALPH"), BLEND_ADD);
        SetOptionF(g_bar_lines[0], TAG("ZOFS"), -1.0);
        g_bar_lines[1] = CreatePlane(Disp2D(0));
        SetPos(g_bar_lines[1], g_center_x, 0.5 + (g_screen_h - bar_h), 0.0, 0);
        SetLocalScale(g_bar_lines[1], g_screen_w, 1.0, 1.0);
        SetColor(g_bar_lines[1], 1.0, 1.0, 1.0, 0.0);
        SetOptionI(g_bar_lines[1], TAG("ALPH"), BLEND_ADD);
        SetOptionF(g_bar_lines[1], TAG("ZOFS"), -1.0);
        update_bars();
    }
    g_logo = CreatePlane(g_bars[0]);
    SetPos(g_logo, 26.0 + -g_center_x + 0.5 * g_logo_w, g_logo_y, 0.0, 0);
    SetScale(g_logo, g_logo_w, g_logo_h, 0.0);
    SetTexture(g_logo, imgGetResource(RES_PSBBLOGO), 0, -1);
    SetColor(g_logo, 1.0, 1.0, 1.0, 0.0);
    AnimColor(g_logo, 1.0, 1.0, 1.0, 1.0, g_fade_time, 0.0, ANIM_FUNC_LINEAR);
    load_common_bg("bg", g_fade_time);
    for (new i; i < 4; i++)
    {
        new text[64];
        get_tag_attr("TEXT", g_button_text_ids[i], "value", text, 64);
        SetSystemParamStr(g_label_params[i], text);
        SetSystemParamStr(g_default_params[i], text);
    }
    wait_ms(5000);
    for (new i; i < 6; i++)
    {
        create_pad_button(i);
    }
    g_busy = CreateNull(g_bars[0]);
    SetPos(g_busy, g_center_x - 30.0, 12.0, 0.0, 0);
    AttachScript(g_busy, _, "busy");
    g_mail_icon = CreatePlane(g_bars[0]);
    SetTexture(g_mail_icon, imgGetResource(RES_BIFF), 0, -1);
    SetPos(g_mail_icon, g_center_x - 70.0, 12.0, 0.0, 0);
    SetScale(g_mail_icon, 34.0, 29.0, 0.0);
    SetColor(g_mail_icon, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_mail_icon, TAG("ALPH"), BLEND_ADD);
    Hide(g_mail_icon);
    g_lock_icon = CreatePlane(g_bars[0]);
    SetTexture(g_lock_icon, imgGetResource(RES_HTTPS), 0, -1);
    SetPos(g_lock_icon, g_center_x - 110.0, 12.0, 0.0, 0);
    SetScale(g_lock_icon, 34.0, 24.0, 0.0);
    SetColor(g_lock_icon, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_lock_icon, TAG("ALPH"), BLEND_ADD);
    Hide(g_lock_icon);
    g_page_label = CreateLabel(g_bars[0]);
    SetPos(g_page_label, 0.0, 12.0, 0.0, 0);
    SetOptionI(g_page_label, TAG("FSIZ"), 14);
    SetOptionI(g_page_label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_page_label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_page_label, TAG("VALN"), ALIGN_CENTER);
    Hide(g_page_label);
    g_button_shown[0] = 1;
    g_button_shown[1] = 1;
    g_button_shown[2] = 1;
    g_button_shown[3] = 1;
    g_mail_shown = 1;
    g_lock_shown = 1;
    set_button_colors();
    place_buttons();
    sleep;
    new changed = 1;
    new moved = 1;
    for (; ; )
    {
        {
            new msg;
            while (RecvMessage(msg))
            {
            }
        }
        update_pad_help(changed);
        sleep;
        {
            new text[32];
            for (new i; i < 4; i++)
            {
                GetSystemParamStr(g_label_params[i], text, 32);
                if (0 != strcmp(g_button_texts[i], text))
                {
                    set_button_text(i, text);
                }
                sleep;
            }
        }
        update_pad_help(changed);
        sleep;
        update_bars();
        sleep;
        {
            new page_no = GetSystemParam(SYS_PAGE_NO);
            new page_max = GetSystemParam(SYS_PAGE_MAX);
            if (page_max != g_page_max || page_no != g_page_no)
            {
                g_page_max = page_max;
                g_page_no = page_no;
                if (g_page_max > 0)
                {
                    new text[4];
                    sprintf(text, "%d/%d", g_page_no, g_page_max);
                    SetOptionS(g_page_label, TAG("UTF8"), text);
                    Show(g_page_label);
                }
                else
                {
                    SetOptionS(g_page_label, TAG("UTF8"), "");
                    Hide(g_page_label);
                }
            }
        }
        sleep;
        for (new i; i < 4; i++)
        {
            new rect[5];
            GetOptionP(g_button_labels[i], TAG("RECT"), rect, 5);
            if (rect[0])
            {
                new Float:width = float(rect[3]) / 64.0;
                if (g_label_w[i] != width)
                {
                    g_label_w[i] = width;
                    g_label_ready[i] = 1;
                    changed = 1;
                    moved = 1;
                }
            }
        }
        sleep;
        update_pad_help(changed);
        if (moved)
        {
            place_buttons();
            moved = 0;
        }
        sleep;
        for (new i; i < 6; i++)
        {
            if (!is_lr_button(i))
            {
                new Float:alpha;
                GetColor(g_button_icons[i], _, _, _, alpha);
                if (g_button_shown[i] && (g_label_ready[i] && alpha < 0.01))
                {
                    Hide(g_button_icons[i]);
                    Hide(g_button_labels[i]);
                    g_button_shown[i] = 0;
                }
            }
            else
            {
                hide_if_faded(g_button_icons[i], g_button_shown[i]);
            }
        }
        sleep;
        {
            new mail = GetSystemParam(SYS_BIFF_EXIST);
            static last_mail;
            if (mail && !last_mail)
            {
                Show(g_mail_icon);
                g_mail_shown = 1;
                if (GetSystemParam(SYS_BIFF_SOUND))
                {
                    sndSePlay(SE_MAIL_ARRIVED);
                }
                new flash = CreatePlane(g_bars[0]);
                SetTexture(flash, imgGetResource(RES_BUSY), 0, -1);
                SetPos(flash, g_center_x - 70.0, 12.0, 0.0, 0);
                SetScale(flash, 10.0, 10.0, 0.0);
                SetColor(flash, 1.0, 1.0, 1.0, 0.0);
                SetOptionI(flash, TAG("ALPH"), BLEND_ADD);
                new Float:color[4] = {0.2, 0.5, 1.0, 3.0};
                AnimScale(flash, 120.0, 10.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(flash, color[0], color[1], color[2], 0.2 * color[3], 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimScale(flash, 70.0, 10.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(flash, color[0], color[1], color[2], 0.3 * color[3], 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimScale(flash, 70.0, 50.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(flash, color[0], color[1], color[2], 0.2 * color[3], 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(flash, color[0], color[1], color[2], 0.0 * color[3], 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_mail_icon, 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                Destroy(flash);
            }
            else if (!mail && last_mail)
            {
                AnimColor(g_mail_icon, 1.0, 1.0, 1.0, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
            }
            last_mail = mail;
        }
        {
            new secure = GetSystemParam(SYS_MAIN_HTTPS) | GetSystemParam(SYS_HTTPS_BUSY);
            static last_secure;
            if (secure && !last_secure)
            {
                Show(g_lock_icon);
                g_lock_shown = 1;
                AnimColor(g_lock_icon, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (!secure && last_secure)
            {
                AnimColor(g_lock_icon, 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            }
            last_secure = secure;
        }
        sleep;
        update_pad_help(changed);
        sleep;
        hide_if_faded(g_mail_icon, g_mail_shown);
        sleep;
        hide_if_faded(g_lock_icon, g_lock_shown);
        sleep;
    }
}

new g_last_bar_alpha; /* NOTE: bug: never set, so update_bars fades the bars again on every call */

/* Hides obj once it has faded out, and clears shown. */
hide_if_faded(obj, &shown)
{
    new Float:alpha;
    GetColor(obj, _, _, _, alpha);
    if (shown && alpha < 0.01)
    {
        Hide(obj);
        shown = 0;
    }
}

place_buttons()
{
    new i;
    i = 2;
    SetPos(g_button_icons[i], -g_center_x + g_button_x[0][i] + g_icon_size / 2.0, -15.0, 0.0, 0);
    SetPos(g_button_labels[i], 14.0 + (-g_center_x + g_button_x[0][i] + g_icon_size / 2.0), -13.0, 0.0, 0);
    i = 1;
    SetPos(g_button_icons[i], -g_center_x + g_button_x[0][i] + g_icon_size / 2.0, -15.0, 0.0, 0);
    SetPos(g_button_labels[i], 14.0 + (-g_center_x + g_button_x[0][i] + g_icon_size / 2.0), -13.0, 0.0, 0);
    i = 3;
    new Float:x = g_screen_w - g_label_w[3] - 24.0 - 14.0;
    SetPos(g_button_icons[i], -g_center_x + x, -15.0, 0.0, 0);
    SetPos(g_button_labels[i], 14.0 + (-g_center_x + x), -13.0, 0.0, 0);
    i = 0;
    SetPos(g_button_icons[i], -g_center_x + g_button_x[0][i] + g_icon_size / 2.0, -15.0, 0.0, 0);
    SetPos(g_button_labels[i], 14.0 + (-g_center_x + g_button_x[0][i] + g_icon_size / 2.0), -13.0, 0.0, 0);
    i = 4;
    SetPos(g_button_icons[i], -g_center_x + g_lr_x[i - 4], 10.0, 0.0, 0);
    i = 5;
    SetPos(g_button_icons[i], -g_center_x + g_lr_x[i - 4], 10.0, 0.0, 0);
}

/* Makes the buttons that SYS_PADHELP turns on opaque at once, and the others transparent. */
set_button_colors()
{
    for (new i; i < 6; i++)
    {
        if (!is_lr_button(i))
        {
            if (g_pad_help & g_button_bits[i] && g_label_ready[i])
            {
                SetColor(g_button_icons[i], 1.0, 1.0, 1.0, 1.0);
                SetColor(g_button_labels[i], 1.0, 1.0, 0.7, 1.0);
            }
            else
            {
                SetColor(g_button_icons[i], 1.0, 1.0, 1.0, 0.0);
                SetColor(g_button_labels[i], 1.0, 1.0, 1.0, 0.0);
            }
        }
        else if (g_pad_help & g_button_bits[i])
        {
            SetColor(g_button_icons[i], 1.0, 1.0, 1.0, 1.0);
        }
        else
        {
            SetColor(g_button_icons[i], 1.0, 1.0, 1.0, 0.0);
        }
    }
}

/* Like set_button_colors, fading. */
fade_buttons()
{
    for (new i; i < 6; i++)
    {
        if (!is_lr_button(i))
        {
            if (g_pad_help & g_button_bits[i] && g_label_ready[i])
            {
                Show(g_button_icons[i]);
                Show(g_button_labels[i]);
                g_button_shown[i] = 1;
                AnimColor(g_button_icons[i], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_button_labels[i], 1.0, 1.0, 1.0, 0.7, 200.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                AnimColor(g_button_icons[i], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_button_labels[i], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        else if (g_pad_help & g_button_bits[i])
        {
            Show(g_button_icons[i]);
            g_button_shown[i] = 1;
            AnimColor(g_button_icons[i], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            AnimColor(g_button_icons[i], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
}

/* Fades the bars to the alpha of SYS_FGBAR_ALPHA. */
update_bars()
{
    new alpha_byte = GetSystemParam(SYS_FGBAR_ALPHA);
    if (alpha_byte != g_last_bar_alpha)
    {
        new Float:alpha = alpha_byte / 255.0;
        AnimColor(g_bars[0], 0.0, 0.0, 0.0, g_bar_alpha * alpha, 300.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_bars[1], 0.0, 0.0, 0.0, g_bar_alpha * alpha, 300.0, 0.0, ANIM_FUNC_LINEAR);
        if (alpha > 0.4)
        {
            alpha = 0.4;
        }
        AnimColor(g_bar_lines[0], 1.0, 1.0, 1.0, g_line_alpha * alpha, 300.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_bar_lines[1], 1.0, 1.0, 1.0, g_line_alpha * alpha, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Follows SYS_PADHELP and the labels of the buttons it turns on; fades the buttons if they changed. */
update_pad_help(&changed)
{
    g_pad_help = GetSystemParam(SYS_PADHELP);
    if (g_pad_help != g_last_pad_help)
    {
        for (new i; i < 4; i++)
        {
            if (g_pad_help & g_button_bits[i] && !(g_last_pad_help & g_button_bits[i]))
            {
                new text[32];
                GetSystemParamStr(g_label_params[i], text, 32);
                if (0 != strcmp(g_button_texts[i], text))
                {
                    set_button_text(i, text);
                    sleep;
                }
            }
        }
        g_last_pad_help = g_pad_help;
        changed = 1;
    }
    if (changed)
    {
        fade_buttons();
        changed = 0;
    }
}
