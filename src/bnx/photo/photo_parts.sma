/*
 * /opt0/bn/script/photo/photo_parts.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Parts of the photo page: the script of its camera and the large photo under the cursor of the thumbnails.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "photodb.inc"

/* The states of the photo of photo_focused_main. */
const IMAGE_NONE = 0;
const IMAGE_DECODING = 1;
const IMAGE_WAITING = 2; /* decoding too, never set */
const IMAGE_SHOWN = 3;
const IMAGE_BROKEN = 4; /* it cannot be decoded */

/* The captions over the photo. */
const CAPTION_NONE = 0;
const CAPTION_FINDING = 1;
const CAPTION_BROKEN = 2;

new Float:g_pi = 3.141592;

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

#include <float>

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

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
new g_caption_labels[2][2];
new g_caption_index;
new Float:g_caption_x;
new Float:g_caption_y;

/* Same as poll_image_decode of imgdec.inc. */
poll_image_decode()
{
    return imgDecSync();
}

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

/* Creates the labels of show_caption, with a shadow if shadow. */
create_captions(shadow)
{
    new i;
    new j;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size(screen_w, screen_h);
        g_caption_x = 0.546875 * screen_w;
        g_caption_y = 0.5 * screen_h;
    }
    i = 0;
    for (; i < 2; i++)
    {
        j = 0;
        for (; j < 2; j++)
        {
            if (!shadow)
            {
                g_caption_labels[i][j] = CreateLabel(Disp2D(0));
            }
            else
            {
                g_caption_labels[i][j] = CreateLabelShadow(Disp2D(0));
            }
            SetPos(g_caption_labels[i][j], g_caption_x, g_caption_y, 0.0, 0);
            SetOptionI(g_caption_labels[i][j], TAG("FONT"), FontID(0));
            SetOptionI(g_caption_labels[i][j], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_caption_labels[i][j], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_caption_labels[i][j], TAG("FSIZ"), 17);
            if (shadow)
            {
                SetOptionP(g_caption_labels[i][j], TAG("SCL4"), g_shadow_colors, 4);
                SetOptionI(g_caption_labels[i][j], TAG("SX64"), 2 * 64);
                SetOptionI(g_caption_labels[i][j], TAG("SY64"), 2 * 64);
            }
            SetColor(g_caption_labels[i][j], 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
}

/* Same as show_caption of string.inc. */
show_caption(line1[] = "", line2[] = "", Float:target_a, Float:duration, Float:delay)
{
    new incoming = g_caption_index;
    g_caption_index = ++g_caption_index & 1;
    new outgoing = g_caption_index;
    SetOptionS(g_caption_labels[incoming][0], TAG("UTF8"), line1);
    SetOptionS(g_caption_labels[incoming][1], TAG("UTF8"), line2);
    if (strlen(line2) > 0)
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y - 12.0, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, 12.0 + g_caption_y, 0.0, 0);
    }
    else
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, g_caption_y, 0.0, 0);
    }
    AnimColor(g_caption_labels[incoming][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[incoming][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][0], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][1], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
}

public camera_focus;     /* the object camera_main keeps the camera on */
public decstate;         /* the photo, DECSTATE_* */

/* Same as set_caption_pos of misc.inc. */
set_caption_pos(Float:x, Float:y)
{
    g_caption_x = x;
    g_caption_y = y;
}

/* The script of the camera of the page: it keeps looking at camera_focus. */
public camera_main()
{
    for (; ; )
    {
        LookAt(Self(), camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
        sleep;
    }
}

/*
 * The large photo under the cursor of the thumbnails. "DCDE" (url) decodes and shows a photo, "ROT" (quarter turns)
 * rotates it, "BUSY" hides it, "FADE" (alpha, time) fades it; "SLEP" (alpha, time, keep) stops the script, fading the
 * photo out unless keep, and "WAKE" (alpha, time, clear) resumes it.
 */
public photo_focused_main()
{
    new Float:screen_w;
    new Float:screen_h;
    new self = Self();
    new awake;
    new state;
    new rotation;
    new caption;
    new shown_caption;
    new Float:scale_w = 1.0;
    new Float:scale_h = 1.0;
    new Float:scale_d = 1.0;
    SetColor(self, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(self, TAG("FDEG"), 1);
    get_screen_size(screen_w, screen_h);
    create_captions(0);
    set_caption_pos(0.546875 * screen_w, 0.5 * screen_h);
    for (; ; )
    {
        new msg;
        new Float:a;
        new Float:b;
        new c;
        new url[256];
        /*
         * "SLEP" (alpha, duration, c: 0 drops the image) and "WAKE" (alpha, duration, c: nonzero clears the caption)
         * stop and resume the part; "BUSY" hides the image; "FADE" (alpha, duration); "ROT" (quarter turns).
         */
        if (FindMessage(TAG("SLEP"), a, b, c))
        {
            awake = 0;
            if (0 == c)
            {
                state = IMAGE_NONE;
                AnimColor(self, 1.0, 1.0, 1.0, a, b, 0.0, ANIM_FUNC_LINEAR);
            }
            shown_caption = CAPTION_NONE;
            show_caption(_, _, 0.0, 400.0, 0.0);
        }
        if (FindMessage(TAG("WAKE"), a, b, c))
        {
            awake = 1;
            if (state == IMAGE_SHOWN)
            {
                AnimColor(self, 1.0, 1.0, 1.0, a, b, 0.0, ANIM_FUNC_LINEAR);
            }
            if (c)
            {
                caption = CAPTION_NONE;
            }
        }
        if (!awake)
        {
            sleep;
            continue;
        }
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("BUSY"))
            {
                state = IMAGE_NONE;
                AnimColor(self, 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                caption = CAPTION_FINDING;
            }
            else if (msg == TAG("FADE"))
            {
                if (state == IMAGE_SHOWN)
                {
                    AnimColor(self, 1.0, 1.0, 1.0, a, b, 0.0, ANIM_FUNC_LINEAR);
                }
            }
            else if (msg == TAG("ROT"))
            {
                rotation = a;
                if (state)
                {
                    SetRot(self, 0.0, 0.0, 0.5 * (float(rotation) * g_pi), 0, 0);
                    if (0 == rotation % 2)
                    {
                        SetLocalScale(self, scale_w, scale_h, scale_d);
                    }
                    else
                    {
                        SetLocalScale(self, scale_h, scale_w, scale_d);
                    }
                }
            }
        }
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE"))
            {
                if (strlen(url) > 0)
                {
                    imgDecStart(url, floatround(256.0, 0), floatround(256.0, 0), -1, 100, 0);
                    AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                    decstate = DECSTATE_PENDING;
                    state = IMAGE_DECODING;
                    caption = CAPTION_FINDING;
                }
                else
                {
                    decstate = DECSTATE_BROKEN;
                    state = IMAGE_BROKEN;
                    caption = CAPTION_BROKEN;
                }
            }
        }
        if (state == IMAGE_WAITING || state == IMAGE_DECODING)
        {
            new ret;
            if (state == IMAGE_WAITING)
            {
                ret = poll_image_decode();
            }
            else
            {
                ret = imgDecSync();
            }
            if (ret == IMG_DONE)
            {
                SetTexture(self, imgTexID(), 0, -1);
                new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                if (aspect > 1.0)
                {
                    scale_w = 256.0;
                    scale_h = 256.0 / aspect;
                }
                else
                {
                    scale_w = 256.0 * aspect;
                    scale_h = 256.0;
                }
                scale_d = 256.0;
                decstate = DECSTATE_SHOWN;
                state = IMAGE_SHOWN;
                SetRot(self, 0.0, 0.0, 0.5 * (float(rotation) * g_pi), 0, 0);
                caption = CAPTION_NONE;
                AnimColor(self, 1.0, 1.0, 1.0, 1.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                if (0 == rotation % 2)
                {
                    SetLocalScale(self, scale_w, scale_h, scale_d);
                }
                else
                {
                    SetLocalScale(self, scale_h, scale_w, scale_d);
                }
            }
            else if (ret == IMG_FAILED)
            {
                decstate = DECSTATE_BROKEN;
                state = IMAGE_BROKEN;
                caption = CAPTION_BROKEN;
            }
        }
        if (caption != shown_caption)
        {
            if (CAPTION_NONE == caption)
            {
                show_caption(_, _, 0.0, 400.0, 0.0);
            }
            else if (caption == CAPTION_FINDING)
            {
                new text[64];
                get_tag_attr("TEXT", "msgphoto_pict_finding", "value", text, 64);
                show_caption(text, _, 1.0, 200.0, 700.0);
            }
            else if (caption == CAPTION_BROKEN)
            {
                new text[64];
                get_tag_attr("TEXT", "msgphoto_pict_corrupt", "value", text, 64);
                show_caption(text, _, 1.0, 400.0, 0.0);
            }
            shown_caption = caption;
        }
        sleep;
    }
    return;
}
