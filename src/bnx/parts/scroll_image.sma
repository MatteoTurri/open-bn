/*
 * /opt0/bn/script/parts/scroll_image.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The image of the argument "image" shown 290 by 46 pixels at a time on the license model: an image taller than
 * that scrolls slowly to its end, fades out and starts again from the top.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include "imgsync.inc"

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
new g_model;

/* Waits for the pending model load; returns its status. */
wait_model_load()
{
    for (; ; )
    {
        new status = modelLoadSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

public main()
{
    g_model = CreateModel(Self());
    SetScale(g_model, 1.0, 1.0, 1.0);
    SetPos(g_model, 0.0, 0.0, 0.0, 0);
    SetColor(g_model, 1.0, 1.0, 1.0, 0.0);
    SetModel(g_model, load_model("/opt0/bn/data/model/license"));
    SetOptionI(g_model, TAG("LIT"), 0);
    SetOptionI(g_model, TAG("PIXT"), 0);
    SetOptionI(g_model, TAG("PIXW"), 15);
    {
        new image[256];
        GetArg("image", image, 256);
        new args[256];
        sprintf(args, "image = %s", image);
        AttachScript(g_model, "scroll_main", "", args);
    }
    for (; ; )
    {
        new msg;
        new new_alpha;
        new Float:duration;
        while (RecvMessage(msg, new_alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                SetPubVar(g_model, "meta_alpha", new_alpha);
                fade_model(duration, 0.0);
            }
        }
        sleep;
    }
}

/* Fades the model to its meta_alpha times its alpha. */
fade_model(Float:duration, Float:delay)
{
    new Float:outer_alpha = GetPubVar(g_model, "meta_alpha");
    new Float:image_alpha = GetPubVar(g_model, "alpha");
    AnimColor(g_model, 1.0, 1.0, 1.0, outer_alpha * image_alpha, duration, delay, ANIM_FUNC_LINEAR);
}

public Float:meta_alpha = 1.0; /* of the model: set by the "FADE" message */
public Float:alpha;            /* of the model: 0.0 while the scroll starts again */

/* The script of the model: decodes the image and scrolls it. */
public scroll_main()
{
    {
        new image[256];
        GetArg("image", image, 256);
        imgDecStart(image, 0, 0, -1, 100, 1);
        if (IMG_DONE != wait_image_decode())
        {
            return -1;
        }
    }
    new top;
    show_part(0, top, 290, 46);
    if (top + 46 < imgHeight())
    {
        for (; ; )
        {
            alpha = 1.0;
            fade(1000.0, 0.0);
            wait_ms(1000);
            wait_ms(1000);
            while (top + 46 < imgHeight())
            {
                top++;
                show_part(0, top, 290, 46);
                sleep;
                sleep;
            }
            alpha = 0.0;
            fade(1000.0, 0.0);
            wait_ms(1000);
            wait_ms(1000);
            top = 0;
            show_part(0, top, 290, 46);
            wait_ms(1000);
        }
    }
    else
    {
        alpha = 1.0;
        fade(1000.0, 0.0);
        wait_ms(1000);
        wait_ms(1000);
    }
    return 0;
}

/* Shows the part of the decoded image at x, y of w by h pixels. */
show_part(x, y, w, h)
{
    imgTrimStart(x, y, w, h);
    if (wait_image_trim() == IMG_DONE)
    {
        SetTexture(Self(), imgTexID(), 0, -1);
    }
}

/* Fades to meta_alpha in duration ms, after delay ms. */
fade(Float:duration, Float:delay)
{
    AnimColor(Self(), 1.0, 1.0, 1.0, meta_alpha * alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Loads a model and returns it. */
load_model(path[])
{
    modelLoadStart(path, 0);
    wait_model_load();
    new info[1];
    modelGetInfo(0, info, 1);
    return info[0];
}
