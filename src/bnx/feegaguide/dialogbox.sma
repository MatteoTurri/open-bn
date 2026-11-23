/*
 * /opt0/bn/script/feegaguide/dialogbox.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The message box of the guide pages: an image with one line of text over the dimmed page, closed with circle.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

#include "imgsync.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

#include "xmlsync.inc"

#include "text.inc"

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
    AttachScript(Terminate, "TerminateThread", "");
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

public Float:width = 300.0; /* unused, but part of the layout */
public Float:height = 300.0; /* unused, but part of the layout */
public Float:meta_alpha = 1.0; /* unused, but part of the layout */
public dialogboxObj; /* the plane of the box */
new g_open;
new g_text;
new g_focus;
new g_backdrop;

public main()
{
    g_backdrop = CreateDefocus(Self());
    SetPos(g_backdrop, 0.0, 0.0, 0.0, 0);
    SetOptionI(g_backdrop, TAG("ALPH"), BLEND_ALPHA);
    SetOptionI(g_backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetColor(g_backdrop, 0.95, 0.95, 0.95, 0.0);
    SetOptionI(g_backdrop, TAG("LEVL"), 8);
    dialogboxObj = CreatePlane(Disp2D(0));
    SetColor(dialogboxObj, 1.0, 1.0, 1.0, 0.0);
    SetPos(dialogboxObj, 312.0, 240.0, 0.0, 0);
    SetOptionI(dialogboxObj, TAG("ALPH"), BLEND_ALPHA);
    SetOptionI(dialogboxObj, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(dialogboxObj, TAG("ZOFS"), -2.0);
    new msg;
    new text_y0;
    new waiting = 1;
    new url[256];
    new packed[256];
    new text[256];
    new tex;
    new sender;
    new pressed;
    new repeated;
    new Float:text_y;
    g_focus = create_focus_plane("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetColor(g_focus, 0.0, 0.0, 0.0, 0.0);
    for (; ; )
    {
        /*
         * The page sends "INIT" (its object), then the strings "POSY" (y of the text), "STRI" (the text) and "OPEN"
         * (the image of the box), each after a "RECV" from here; a last "RECV" tells it the box is closed.
         */
        wait_message(TAG("INIT"), sender);
        text_y = float(text_y0); /* redundant as written */
        SendMessage(sender, TAG("RECV"), 0, 0, 0, 0);
        while (waiting)
        {
            if (RecvMessageStr(msg, packed, 256) && msg == TAG("POSY"))
            {
                break;
            }
            sleep;
        }
        strunpack(text, packed);
        text_y = float(atoi(text));
        SendMessage(sender, TAG("RECV"), 0, 0, 0, 0);
        while (waiting)
        {
            if (RecvMessageStr(msg, packed, 256) && msg == TAG("STRI"))
            {
                break;
            }
            sleep;
        }
        strunpack(text, packed);
        imgDecStringStart(text, 15, 64, 64, 640);
        if (IMG_DONE != wait_image_decode())
        {
            terminate();
        }
        g_text = CreatePlane(dialogboxObj);
        SetTexture(g_text, imgTexID(), 0, -1);
        SetPos(g_text, 0.0, text_y, 0.0, 0);
        SetScale(g_text, float(imgTexWidth()), float(imgTexHeight()), 1.0);
        SetColor(g_text, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(g_text, TAG("ZSRT"), ZSORT_OVERLAY);
        SendMessage(sender, TAG("RECV"), 0, 0, 0, 0);
        while (waiting)
        {
            if (RecvMessageStr(msg, packed, 256) && msg == TAG("OPEN"))
            {
                break;
            }
            sleep;
        }
        strunpack(url, packed);
        imgDecStart(url, 0, 0, -1, 100, 0);
        if (IMG_DONE != wait_image_decode())
        {
            terminate();
        }
        tex = imgTexID();
        SetLocalScale(dialogboxObj, float(imgTexWidth()), float(imgTexHeight()), 1.0);
        SetTexture(dialogboxObj, tex, 0, -1);
        AnimColor(g_text, 1.0, 1.0, 1.0, 1.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(dialogboxObj, 1.0, 1.0, 1.0, 1.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_backdrop, 0.95, 0.95, 0.95, 1.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
        while (ANIM_DONE != AnimGetState(dialogboxObj, ANIM_COLOR))
        {
            sleep;
        }
        g_open = 1;
        set_focus(g_focus, g_text, 0, 0.5, 60.0, 60.0, 0.0, 1, 1);
        while (g_open)
        {
            PadState(_, pressed, _, repeated, -1);
            if (repeated & PAD_CIRCLE)
            {
                g_open = 0;
                sndSePlay(SE_OPEN);
            }
            sleep;
        }
        AnimColor(dialogboxObj, 1.0, 1.0, 1.0, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_text, 1.0, 1.0, 1.0, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_backdrop, 0.95, 0.95, 0.95, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
        set_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        while (ANIM_DONE != AnimGetState(dialogboxObj, ANIM_COLOR))
        {
            sleep;
        }
        SetTexture(dialogboxObj, 0, 0, -1);
        imgReleaseTex(tex);
        SendMessage(sender, TAG("RECV"), 0, 0, 0, 0);
    }
    return;
}
