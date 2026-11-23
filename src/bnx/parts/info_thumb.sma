/*
 * /opt0/bn/script/parts/info_thumb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The picture of an information menu: the image of the url sent with "DCDE", or the texture sent with TEX, fitted
 * in a 78-pixel square; "FNSH" fades it out and destroys it.
 */

#include <core>
#include <bn>

#pragma dynamic 256

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgdec.inc"

/* Values of state: the image. */
enum
{
    THUMB_IDLE,         /* shown, or nothing to show yet */
    THUMB_DECODING,
    THUMB_DECODING_TM2,
    THUMB_NONE          /* empty url, or the image failed */
};

new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
public img_org_w; /* the size of the decoded image, -1 when it failed */
public img_org_h; /* with img_org_w */

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

#include "string.inc"

/* Sets the texture of the picture, of width / height aspect, fitted in 78 pixels. */
set_thumb(tex, Float:aspect)
{
    SetTexture(Self(), tex, 0, -1);
    if (0.0 == aspect)
    {
        aspect = 1.0;
    }
    if (aspect > 1.0)
    {
        SetLocalScale(Self(), 78.0, 78.0 / aspect, 78.0);
    }
    else
    {
        SetLocalScale(Self(), 78.0 * aspect, 78.0, 78.0);
    }
}

public info_thumb()
{
    new obj = Self();
    new state;
    new finished;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    SetColor(obj, 1.0, 1.0, 1.0, 0.0);
    SetPos(obj, 184.0 * screen_w / 640.0, 138.0 * screen_h / 480.0, 0.0, 0);
    for (; ; )
    {
        new msg;
        new url[128];
        while (RecvMessageStr(msg, url, 128))
        {
            if (msg == TAG("DCDE"))
            {
                if (has_extension(url, "tm2"))
                {
                    start_image_decode2(url);
                    state = THUMB_DECODING_TM2;
                }
                else if (strlen(url) > 0)
                {
                    imgDecStart(url, floatround(256.0, 0), floatround(256.0, 0), 0, 100, 0);
                    state = THUMB_DECODING;
                }
                else
                {
                    state = THUMB_NONE;
                }
            }
        }
        new tex;
        new Float:aspect;
        while (FindMessage(TAG("TEX"), tex, aspect))
        {
            set_thumb(tex, aspect);
        }
        while (FindMessage(TAG("FNSH"), tex))
        {
            finished = 1;
        }
        if (state == THUMB_DECODING_TM2 || state == THUMB_DECODING)
        {
            SetColor(obj, 1.0, 1.0, 1.0, 0.0);
            new status;
            if (state == THUMB_DECODING_TM2)
            {
                status = poll_image_decode();
            }
            else
            {
                status = imgDecSync();
            }
            if (status == IMG_DONE)
            {
                printf("image dec end !!!!!\n");
                img_org_w = imgTexOrgWidth();
                img_org_h = imgTexOrgHeight();
                new Float:img_aspect = 1.0 * img_org_w / img_org_h;
                set_thumb(imgTexID(), img_aspect);
                state = THUMB_IDLE;
                AnimColor(obj, 1.0, 1.0, 1.0, 1.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (status == -1)
            {
                img_org_w = -1;
                img_org_h = -1;
                state = THUMB_NONE;
            }
        }
        if (finished)
        {
            break;
        }
        sleep;
    }
    AnimColor(obj, 1.0, 1.0, 1.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(1000);
    Destroy(obj);
    return;
}
