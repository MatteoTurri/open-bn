/*
 * /opt0/bn/script/guide/common/texture_obj.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * An image button of the guide: shows the image of the argument "btn" on the object btn, dimmed while it does
 * not have the focus, and blinks it when the circle button ("RRHT") is pushed on it.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "xmlsync.inc"

#include "string.inc"

public Terminate; /* the object whose thread (TerminateThread) leaves the page */
public isTerminate; /* set by CallTerminate: TerminateThread leaves the page */

/* Goes back to the top menu and closes the scene. */
public terminate()
{
    printf("******************************\n");
    printf("terminate callback\n");
    printf("******************************\n");
    xmlLoadStart("$$BUILDIN_TOPMENU$$", _, _, XMLLOAD_MAIN);
    wait_xml_load();
    Abort(Scene(0));
}

/* Stops this script and tells TerminateThread to leave the page. */
public CallTerminate()
{
    Abort(Self());
    for (; ; )
    {
        isTerminate = 1;
        sleep;
    }
}

/* Creates the Terminate object, whose thread goes back to the top menu once isTerminate is set. */
public CreateTerminate(parent)
{
    Terminate = CreateNull(parent);
    isTerminate = 0;
    AttachScript(Terminate, "TerminateThread", "", "");
}

/* Waits until isTerminate is set, then leaves the page like terminate. */
public TerminateThread()
{
    new msg;
    for (; ; )
    {
        if (isTerminate)
        {
            break;
        }
        while (RecvMessage(msg))
        {
        }
    }
    printf("******************************\n");
    printf("terminate callback\n");
    printf("******************************\n");
    xmlLoadStart("$$BUILDIN_TOPMENU$$", _, _, XMLLOAD_MAIN);
    wait_xml_load();
    Abort(Scene(0));
}

public manager; /* told "DCOK" once the image is decoded */
public focus;   /* the focus object */
public active;  /* has the focus */
public cursor; /* unused, but part of the layout */
public alpha_blend_mode = BLEND_ALPHA; /* the "ALPH" option of btn */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public btn;           /* the object that shows the image */
public expand = 1;    /* fit the image in the object keeping its aspect, instead of showing it at its size */
public read_num = 10; /* decode attempts, 0 for no limit */

main()
{
    new tex_size = 32;
    {
        new arg[32];
        if (GetArg("tex_size", arg, 32))
        {
            tex_size = atoi(arg);
        }
    }
    new name[64];
    new src[64];
    sprintf(name, "btn");
    if (GetArg(name, src, 64))
    {
        SetLocalScale(btn, 70.0, 70.0, 70.0);
        SetColor(btn, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(btn, TAG("ALPH"), alpha_blend_mode);
        decode_texture(btn, src, tex_size, tex_size, -1);
    }
    fade(100.0, 0.0);
    wait_ms(100);
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        /*
         * "FOCS" (how): the focus came here; "FADE" (alpha, duration); "TEX" (_, texture): a new texture;
         * "PUSH" (button): a button was pressed on it.
         */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                handle_focus(a);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                fade(b, 0.0);
            }
            else if (msg == TAG("TEX"))
            {
                SetTexture(btn, b, 0, -1);
            }
            else if (msg == TAG("PUSH") && a == TAG("RRHT"))
            {
                sndSePlay(SE_DECIDE);
                AnimColor(btn, 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(btn, 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(btn, 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(btn, 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(btn, 1.0, 1.0, 1.0, 0.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                AnimColor(btn, 1.0, 1.0, 1.0, 1.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(100);
                wait_ms(100);
            }
        }
        sleep;
    }
    return;
}

/* Fades btn to meta_alpha, or to half of it without the focus. */
fade(Float:duration, Float:delay)
{
    new Float:alpha = 0.5;
    if (active)
    {
        alpha = 1.0;
    }
    AnimColor(btn, 1.0, 1.0, 1.0, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Takes the focus, or loses it when how is "FOUT". */
handle_focus(how)
{
    active = 1;
    if (how == TAG("FOUT"))
    {
        focus_out();
        return;
    }
    if (how == TAG("FRST"))
    {
    }
    sndSePlay(SE_CURSOR);
    set_focus(focus, btn, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    fade(100.0, 0.0);
}

new g_tex;

focus_out()
{
    active = 0;
    fade(100.0, 40.0);
}

/*
 * Decodes the image url onto obj, trying read_num times, and tells the manager "DCOK" in any case. The object
 * shrinks to the aspect of the image, or with expand off takes the size of the image.
 */
decode_texture(obj, url[], width, height, image_mode)
{
    if (0 == strlen(url))
    {
        return;
    }
    new path[256];
    sprintf(path, url);
    new tries;
    while (tries < read_num || 0 == read_num)
    {
        imgDecStart(path, width, height, image_mode, 100, 0);
        if (IMG_DONE != wait_image_decode())
        {
            terminate();
        }
        g_tex = imgTexID();
        if (0 != g_tex)
        {
            SendMessage(manager, TAG("DCOK"), TAG("DCOK"), 0, 0, 0);
            SetTexture(obj, g_tex, 0, -1);
            new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
            if (expand)
            {
                new Float:scale[3];
                GetLocalScale(obj, scale[0], scale[1], scale[2]);
                if (aspect > 1.0)
                {
                    SetLocalScale(obj, scale[0], scale[1] / aspect, scale[2]);
                }
                else
                {
                    SetLocalScale(obj, scale[0] * aspect, scale[1], scale[2]);
                }
            }
            else
            {
                SetLocalScale(obj, float(imgTexOrgWidth()), float(imgTexOrgHeight()), 1.0);
            }
            return;
        }
        tries++;
    }
    SendMessage(manager, TAG("DCOK"), TAG("DCOK"), 0, 0, 0);
    return;
}
