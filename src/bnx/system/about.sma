/*
 * /opt0/bn/script/system/about.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The about page: the version over the title image, then the credits scrolling up (up and down change the
 * speed) until their end or cross.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Appends src to dest (maxlen cells), packed or unpacked like dest. */
append_string(dest[], src[], maxlen)
{
    new dest_len;
    new src_len;
    new i;
    new room;
    dest_len = strlen(dest);
    src_len = strlen(src);
    if (is_packed(dest))
    {
        room = maxlen * 4 - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src[i];
            }
        }
        new pad = 4 - (dest_len + i) % 4;
        for (new j; j < pad; j++)
        {
            dest{dest_len + i + j} = 0;
        }
    }
    else
    {
        room = maxlen - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src[i];
            }
        }
        dest[dest_len + i] = 0;
    }
    return;
}

#include "text.inc"

/* Copies src into dest (maxlen cells) with every occurrence of from replaced by to. */
replace_all(dest[], src[], from[], to[], maxlen)
{
    new count;
    new from_len = strlen(from);
    new to_len = strlen(to);
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, from, src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return count;
        }
        copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
        dest_pos = dest_pos + (found - src_pos);
        append_string(dest, to, maxlen);
        dest_pos = dest_pos + to_len;
        src_pos = found + from_len;
    }
    return 0;
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

#include "misc.inc"

new g_black;
new g_image;
new g_version_label;

main()
{
    SystemCommand(SYSCMD_HIDE_WINDOW, 1);
    SetSystemParam(SYS_FG_ALPHA, 0);
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size2(screen_w, screen_h);
    g_black = CreatePlane(Disp2D(0));
    SetPos(g_black, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
    SetLocalScale(g_black, screen_w, screen_h, 1.0);
    SetColor(g_black, 0.0, 0.0, 0.0, 0.0);
    AnimColor(g_black, 0.0, 0.0, 0.0, 1.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(g_black, TAG("ZSRT"), ZSORT_BACKGROUND);
    g_image = CreatePlane(Disp2D(0));
    SetPos(g_image, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
    SetLocalScale(g_image, screen_w, screen_h, 1.0);
    SetColor(g_image, 1.0, 1.0, 1.0, 0.0);
    {
        new version[25];
        new format[25];
        new unused[3]; /* unused, but part of the layout */
        get_tag_attr("TEXT", "version", "value", format, 25);
        replace_all(version, format, "%VER", "0.32", 25);
        new Float:x;
        {
            new value[8];
            get_tag_attr("TEXT", "version_x", "value", value, 8);
            x = atof(value) * screen_w;
        }
        new Float:y;
        {
            new value[8];
            get_tag_attr("TEXT", "version_y", "value", value, 8);
            y = atof(value) * screen_h;
        }
        g_version_label = CreateLabel(Disp2D(0));
        SetPos(g_version_label, x, y, 0.0, 0);
        SetColor(g_version_label, 0.5, 0.5, 0.5, 0.0);
        SetOptionS(g_version_label, TAG("UTF8"), version);
        SetOptionI(g_version_label, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_version_label, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_version_label, TAG("FSIZ"), 18);
        SetOptionF(g_version_label, TAG("ZOFS"), 1.0);
    }
    new url[64];
    get_tag_attr("IMG", "image", "src", url, 64);
    imgDecStart(url, 0, 0, -1, 100, 1);
    wait_ms(1000);
    if (IMG_DONE != wait_image_decode())
    {
        Destroy(Self());
    }
    printf("image size = %d, %d \n", imgWidth(), imgHeight());
    new view_w = get_smaller(imgWidth(), floatround(screen_w, 0));
    new view_h = get_smaller(imgHeight(), floatround(screen_h, 0));
    show_image_rect(0, 0, view_w, 448, 0, (floatround(screen_h, 0) - 448) / 2, 0.0);
    fade_image(1.0, float(2000));
    AnimColor(g_version_label, 0.5, 0.5, 0.5, 1.0, float(2000), 0.0, ANIM_FUNC_LINEAR);
    wait_ms(2000);
    wait_unless_cross(2000);
    fade_image(0.0, float(2000));
    AnimColor(g_version_label, 0.5, 0.5, 0.5, 0.0, float(2000), 0.0, ANIM_FUNC_LINEAR);
    wait_unless_cross(2000);
    new src_x;
    new src_y = 260;
    new last_x = src_x;
    new last_y = src_y;
    show_image_rect(src_x, src_y, view_w, 448, 0, (floatround(screen_h, 0) - 448) / 2, 0.0);
    fade_image(1.0, float(2000));
    wait_unless_cross(2000);
    wait_unless_cross(2000);
    fade_image(0.0, float(2000));
    wait_unless_cross(2000);
    fade_image(1.0, 0.0);
    for (; ; )
    {
        new credits_top = 700; /* the first row of the credits, which rise from below the screen */
        new image_h = imgHeight();
        new held;
        new pressed;
        PadState(held, pressed, _, _, -1);
        if (pressed & PAD_CROSS)
        {
            fade_image(0.0, float(1000));
            wait_ms(1000);
            break;
        }
        if (held & PAD_UP && src_y > 260)
        {
            src_y = src_y + -4;
        }
        if (held & PAD_DOWN)
        {
            src_y = src_y + 2;
        }
        src_y = src_y + 2;
        src_y = clamp(src_y, 260, image_h);
        if (src_y >= image_h)
        {
            break;
        }
        if (src_x != last_x || src_y != last_y)
        {
            if (src_y < credits_top)
            {
                new gap = credits_top - src_y;
                show_image_rect(src_x, src_y + gap, view_w, view_h - gap, 0, gap, 1.0);
            }
            else if (src_y + view_h > image_h)
            {
                new overflow = src_y + view_h - image_h;
                show_image_rect(src_x, src_y, view_w, view_h - overflow, 0, 0, 1.0);
            }
            else
            {
                show_image_rect(src_x, src_y, view_w, view_h, 0, 0, 1.0);
            }
        }
        sleep;
        last_x = src_x;
        last_y = src_y;
    }
    AnimColor(g_black, 0.0, 0.0, 0.0, 0.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
    wait_ms(1000);
    Destroy(Self());
}

/* Shows the w by h rectangle of the decoded image at (src_x, src_y) with its top left corner at (x, y). */
show_image_rect(src_x, src_y, w, h, x, y, Float:alpha)
{
    imgTrimStart(src_x, src_y, w, h);
    if (wait_image_trim() == IMG_DONE)
    {
        SetTexture(g_image, imgTexID(), 0, -1);
    }
    SetPos(g_image, float(x) + w / 2.0, float(y) + h / 2.0, 0.0, 0);
    SetLocalScale(g_image, float(w), float(h), 1.0);
    SetColor(g_image, 1.0, 1.0, 1.0, 1.0 * alpha);
}

fade_image(Float:alpha, Float:duration)
{
    AnimColor(g_image, 1.0, 1.0, 1.0, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

get_smaller(a, b)
{
    if (a < b)
    {
        return a;
    }
    return b;
}

/* Waits ms (at 60 frames a second); cross fades the page out and closes it. */
wait_unless_cross(ms)
{
    new frames = ms * 60 / 1000;
    for (new i; i < frames; i++)
    {
        new pressed;
        PadState(_, pressed, _, _, -1);
        if (pressed & PAD_CROSS)
        {
            fade_image(0.0, float(1000));
            if (g_version_label)
            {
                AnimColor(g_version_label, 0.5, 0.5, 0.5, 0.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
            }
            wait_ms(1000);
            AnimColor(g_black, 0.0, 0.0, 0.0, 0.0, float(1000), 0.0, ANIM_FUNC_LINEAR);
            wait_ms(1000);
            Destroy(Self());
        }
        sleep;
    }
}
