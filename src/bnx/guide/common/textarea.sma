/*
 * /opt0/bn/script/guide/common/textarea.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Scripts the guide pages attach to their objects: one turns a text into a texture, one moves the focus.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

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

public Float:target_y = -1.0; /* unused, but part of the layout */

/*
 * Renders the string of the "TEXT" message it receives as the texture of its object, at most "width" pixels wide,
 * with the arguments "fontsize" and "align" (ALIGN_LEFT puts the left edge of the text on the object's origin).
 */
public StrTexDecWait()
{
    new arg[64];
    new text[64];
    new font_size;
    new align;
    new Float:y;
    new Float:x;
    new Float:z;
    new Float:tex_w;
    new Float:max_w;
    {
        new msg;
        RecvMessageStr(msg, text, 64);
        if (msg != TAG("TEXT"))
        {
            terminate();
        }
    }
    GetArg("fontsize", arg, 64);
    font_size = atoi(arg);
    GetArg("align", arg, 64);
    align = atoi(arg);
    GetArg("width", arg, 64);
    max_w = atof(arg);
    imgDecStringStart(text, font_size, 64, 64, 640);
    if (IMG_DONE != wait_image_decode())
    {
        terminate();
    }
    SetTexture(Self(), imgTexID(), 0, -1);
    tex_w = float(imgTexWidth());
    if (tex_w > max_w)
    {
        tex_w = max_w;
    }
    SetLocalScale(Self(), tex_w, float(imgTexHeight()), 1.0);
    GetPos(Self(), x, y, z, 0);
    if (ALIGN_LEFT == align)
    {
        x = tex_w / 2.0;
    }
    else if (align == ALIGN_CENTER)
    {
        x = 0.0;
    }
    SetPos(Self(), x, y, z, 0);
    Abort(Self());
}

/* Moves the focus object of the argument "focus" onto the object "focused", once the script of that object ends. */
public SetFocusDecWait()
{
    new arg[64];
    new focus_obj;
    new target;
    new Float:move_time = 150.0;
    new smooth = 1;
    GetArg("focus", arg, 64);
    focus_obj = atoi(arg);
    GetArg("focused", arg, 64);
    target = atoi(arg);
    if (GetArg("move_time", arg, 64))
    {
        move_time = atof(arg);
    }
    if (GetArg("smooth", arg, 64))
    {
        if (!strcmp(arg, "true"))
        {
            smooth = 1;
        }
        else
        {
            smooth = 0;
        }
    }
    while (SCRIPT_NONE != ScriptState(target))
    {
        sleep;
    }
    set_focus(focus_obj, target, 0, 0.5, 60.0, 60.0, move_time, smooth, 1);
    Abort(Self());
}
