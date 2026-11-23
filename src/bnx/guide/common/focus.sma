/*
 * /opt0/bn/script/guide/common/focus.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The focus frame of the guide: a 3x3 plane that moves onto the object in focused, takes its size (or the size of
 * its text, for class "LABL") and blinks there; it declares its natives itself instead of including bn.inc.
 */

#pragma dynamic 128

native FrameRate();
native swapchars(c);
native floatcmp(Float:fOne, Float:fTwo);
native Self();
native SetOptionP(obj, option, const Float:data[], cells);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native CreateNull(parent);
native Hide(obj);
native HideChildren(obj);
native AttachScript(obj, const func[], const file[] = "", const arg[] = "");
native SetPubVar(obj, const name[], value);
native RecvMessage(&msg, &a=0, &b=0, &c=0, &d=0);
native Destroy(obj);
native AnimScale(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native AnimModMove(obj, Float:x, Float:y, Float:z);
native Float:floatdiv(Float:dividend, Float:divisor);
native Float:floatadd(Float:dividend, Float:divisor);
native Scene(obj);
native GetOffPos(obj, const Float:x[], &Float:y, &Float:z, Float:off_x, Float:off_y, Float:off_z, ref);
native GetOptionP(obj, option, const data[], cells);
native GetOptionI(obj, option, &value);
native Float:float(value);
native Float:floatmul(Float:oper1, Float:oper2);
native Parent(obj);
native GetPubVar(obj, const name[]);
native GetScale(obj, &Float:x, &Float:y, &Float:z);
native AnimGetState(obj, anim);
native AnimModScale(obj, Float:x, Float:y, Float:z);
native GetPos(obj, &Float:x, &Float:y, &Float:z, ref);
native random(max);
native Float:floatsub(Float:oper1, Float:oper2);
native AnimMoveLine(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native GetRot(obj, &Float:x, &Float:y, &Float:z, ref, order);
native AnimRotate(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, order);
native floatround(Float:value, method=0);
native GetLocalScale(obj, &Float:x, &Float:y, &Float:z);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native SetRot(obj, Float:x, Float:y, Float:z, ref, order);
native SetScale(obj, Float:x, Float:y, Float:z);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;
native Float:operator/(Float:oper1, Float:oper2) = floatdiv;
native Float:operator+(Float:oper1, Float:oper2) = floatadd;
native Float:operator-(Float:oper1, Float:oper2) = floatsub;

forward Float:float_abs(Float:value);

/* As in bn.inc. */
const ANIM_FUNC_LINEAR = 1;
const ANIM_FUNC_SLOWER = 2;
const ANIM_FUNC_ANGULER = 5;
const ANIM_COLOR = 3;
const ANIM_DONE = 0;
const ALIGN_LEFT = 0;
const ALIGN_CENTER = 1;

/* Values of animation_mode. */
enum
{
    ANIMATION_STAY,      /* stays on the focused object */
    ANIMATION_HIDDEN,    /* nothing focused: drifts around, shrunk */
    ANIMATION_MOVE,      /* moves onto the focused object, then stays */
    ANIMATION_MOVE_LABEL /* the same, sized to the text of a label */
};

public focused;                 /* the object to frame, 0 for none */
public class;                   /* "LABL" to frame the text of a label */
public Float:zoffs; /* depth offset of the frame from the object */
public Float:sharpness = 0.5;   /* margin of the corners of the frame */
public Float:offs_width = 60.0; /* added to the size of the object */
public Float:offs_height = 60.0; /* added to the height of the object */
public smooth = 1;              /* animate the moves; without it the frame jumps */
public Float:move_time = 150.0; /* duration of a move, in ms */
public blink = 1; /* the frame blinks while it stays on an object */
new Float:g_shrunk_size = 50.0; /* size of the frame halfway through a move */
new Float:g_base_alpha = 0.4;
new Float:g_hidden_alpha;
new Float:g_moving_alpha;
new Float:g_still_height = 0.7; /* height ratio of a frame that does not blink */

/* Same as wait_ms of wait.inc. */
wait_ms(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

stock Float:operator-(Float:oper) return oper^Float:0x80000000;

stock bool:operator>(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) > 0;

main()
{
    {
        new Float:frame_margin[4] = {0.2, 0.2, 0.2, 0.2};
        SetOptionP(Self(), TAG("MGN"), frame_margin, 4);
        new Float:tex_margin[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(Self(), TAG("MGNT"), tex_margin, 4);
    }
    SetColor(Self(), 1.0, 1.0, 1.0, 0.0);
    new animator = CreateNull(Self());
    Hide(animator);
    HideChildren(animator);
    new margin_obj = CreateNull(Self());
    Hide(margin_obj);
    HideChildren(margin_obj);
    while (!focused)
    {
        sleep;
    }
    place_on_target(1, Self(), focused, 100.0);
    sleep;
    AttachScript(animator, "animation_main");
    SetPubVar(animator, "class", class);
    SetPubVar(animator, "animation_switch", 1);
    SetPubVar(animator, "animation_mode", ANIMATION_STAY);
    AttachScript(margin_obj, "margin");
    new last_focused = -1;
    for (; ; )
    {
        new msg;
        while (RecvMessage(msg))
        {
            /* "KILL": the frame shrinks, fades out and destroys itself. */
            if (msg == TAG("KILL"))
            {
                Destroy(animator);
                Destroy(margin_obj);
                AnimScale(Self(), 10.0, 10.0, 10.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, 1200.0, 0.0, ANIM_FUNC_ANGULER);
                wait_ms(1200);
                Destroy(Self());
            }
        }
        if (focused != last_focused)
        {
            if (focused)
            {
                new Float:pos[3];
                get_focus_pos(focused, pos, pos[1], pos[2]);
                AnimMoveHermite(Self(), pos[0], pos[1], pos[2], move_time, 0.0, ANIM_FUNC_ANGULER, 0.2);
                SetPubVar(animator, "class", class);
                SetPubVar(animator, "animation_mode", class == TAG("LABL") ? ANIMATION_MOVE_LABEL : ANIMATION_MOVE);
            }
            else
            {
                SetPubVar(animator, "animation_mode", ANIMATION_HIDDEN);
            }
            SetPubVar(animator, "animation_switch", 1);
        }
        last_focused = focused;
        if (focused)
        {
            new Float:pos[3];
            get_focus_pos(focused, pos, pos[1], pos[2]);
            AnimModMove(Self(), pos[0], pos[1], pos[2]);
        }
        sleep;
    }
}

Float:float_abs(Float:value)
{
    return value > 0.0 ? value : -value;
}

/* Gets where the frame goes on target, in the scene: its origin, or the center of the text of a label. */
get_focus_pos(target, Float:pos[], &Float:y, &Float:z)
{
    if (!target)
    {
        return -1;
    }
    if (class != TAG(""))
    {
        new Float:rect[4];
        get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
        if (class == TAG("LABL"))
        {
            GetOffPos(target, pos, y, z, rect[0] + rect[2] / 2.0, -float_abs(rect[1]) + rect[3] / 2.0, zoffs, Scene(0));
        }
    }
    else
    {
        GetOffPos(target, pos, y, z, 0.0, 0.0, zoffs, Scene(0));
    }
    return 0;
}

/* Gets the rectangle of the text of obj, cut to its "W64 " width limit, scaled like obj. */
get_text_rect(obj, &Float:x, &Float:y, &Float:w, &Float:h)
{
    if (!obj)
    {
        return;
    }
    new rect[5];
    GetOptionP(obj, TAG("RECT"), rect, 5);
    if (!rect[0])
    {
        rect[1] = 0;
        rect[2] = 0;
        rect[3] = 0;
        rect[4] = 0;
    }
    else
    {
        new width_limit;
        GetOptionI(obj, TAG("W64 "), width_limit);
        if (width_limit > 0 && rect[3] > width_limit)
        {
            new halign;
            GetOptionI(obj, TAG("HALN"), halign);
            if (ALIGN_LEFT != halign)
            {
                new excess = rect[3] - width_limit;
                if (halign == ALIGN_CENTER)
                {
                    excess = excess / 2;
                }
                rect[1] += excess;
            }
            rect[3] = width_limit;
        }
        x = float(rect[1]) / 64.0;
        y = float(rect[2]) / 64.0;
        w = float(rect[3]) / 64.0;
        h = float(rect[4]) / 64.0;
    }
    new Float:scale[3];
    get_total_scale(obj, scale[0], scale[1], scale[2]);
    x = x * scale[0];
    y = y * scale[1];
    w = w * scale[0];
    h = h * scale[1];
}

/* Keeps the corners of the parent frame round while it is resized. */
public margin()
{
    for (; ; )
    {
        new Float:sharp = GetPubVar(Parent(0), "sharpness");
        new Float:scale[3];
        GetScale(Parent(0), scale[0], scale[1], scale[2]);
        new Float:frame_margin[4];
        if (scale[0] > scale[1])
        {
            frame_margin[0] = frame_margin[1] = sharp * scale[1] / scale[0];
            frame_margin[2] = frame_margin[3] = sharp;
        }
        else
        {
            frame_margin[2] = frame_margin[3] = sharp * scale[0] / scale[1];
            frame_margin[0] = frame_margin[1] = sharp;
        }
        SetOptionP(Parent(0), TAG("MGN"), frame_margin, 4);
        sleep;
    }
}

public animation_mode; /* ANIMATION_* */
public animation_switch; /* set to restart the animation; cleared when it starts */

/* Runs the animation of animation_mode on the parent frame, anew each time animation_switch is set. */
public animation_main()
{
    for (; ; )
    {
        if (!GetPubVar(Parent(0), "focused"))
        {
            animate_hidden();
        }
        else if (GetPubVar(Parent(0), "smooth"))
        {
            switch (animation_mode)
            {
                case ANIMATION_STAY:
                {
                    animate_stay();
                }
                case ANIMATION_HIDDEN:
                {
                    animate_hidden();
                }
                case ANIMATION_MOVE:
                {
                    animate_move();
                }
                case ANIMATION_MOVE_LABEL:
                {
                    animate_move_label();
                }
            }
        }
        else
        {
            place_on_target(0, Parent(0), GetPubVar(Parent(0), "focused"), 0.0);
            animation_mode = ANIMATION_STAY;
            animate_stay();
        }
        animation_switch = 0;
        sleep;
    }
}

/* Keeps the frame on the focused object, blinking; on a label it follows the size of the text. */
animate_stay()
{
    new Float:extra_w = GetPubVar(Parent(0), "offs_width");
    new Float:extra_h = GetPubVar(Parent(0), "offs_height");
    new brighten = 1;
    new target = wait_focused(200);
    new blinking = GetPubVar(Parent(0), "blink");
    if (!target)
    {
        return -1;
    }
    for (; ; )
    {
        if (animation_switch)
        {
            return 0;
        }
        if (ANIM_DONE == AnimGetState(Parent(0), ANIM_COLOR) && blinking)
        {
            if (brighten)
            {
                brighten = 0;
                AnimColor(Parent(0), 1.0, 1.0, 1.0, 0.8 * g_base_alpha, 1200.0, 0.0, ANIM_FUNC_ANGULER);
            }
            else
            {
                brighten = 1;
                AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.2 * g_base_alpha, 1200.0, 0.0, ANIM_FUNC_ANGULER);
            }
        }
        if (target && class == TAG("LABL"))
        {
            new Float:rect[4];
            get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
            if (blinking)
            {
                AnimModScale(Parent(0), rect[2] + extra_w, rect[3] + extra_h, 1.0);
            }
            else
            {
                AnimModScale(Parent(0), rect[2] + extra_w, (rect[3] + extra_h) * g_still_height, 1.0);
            }
        }
        sleep;
    }
    return 0;
}

/* Shrinks the frame and lets it drift around its place. */
animate_hidden()
{
    AnimScale(Parent(0), 20.0, 20.0, 20.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(Parent(0), 1.0, 1.0, 1.0, g_hidden_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
    new Float:pos[3];
    new Float:home[3];
    new Float:velocity[3];
    GetPos(Parent(0), pos[0], pos[1], pos[2], 0);
    home[0] = pos[0];
    home[1] = pos[1];
    home[2] = pos[2];
    velocity[0] = float(random(15)) / 10.0;
    velocity[1] = float(random(15)) / 10.0;
    velocity[2] = float(random(15)) / 10.0;
    for (; ; )
    {
        if (animation_switch)
        {
            return;
        }
        if (home[0] > pos[0])
        {
            velocity[0] += 0.01;
        }
        else
        {
            velocity[0] -= 0.01;
        }
        if (home[1] > pos[1])
        {
            velocity[1] += 0.01;
        }
        else
        {
            velocity[1] -= 0.01;
        }
        if (home[2] > pos[2])
        {
            velocity[2] += 0.01;
        }
        else
        {
            velocity[2] -= 0.01;
        }
        pos[0] += velocity[0];
        pos[1] += velocity[1];
        pos[2] += velocity[2];
        AnimMoveLine(Parent(0), pos[0], pos[1], pos[2], 100.0, 0.0, ANIM_FUNC_LINEAR);
        sleep;
    }
    return;
}

/* Shrinks the frame on the way to the focused object, then grows it to the size of the object. */
animate_move()
{
    new target = GetPubVar(Parent(0), "focused");
    new Float:extra_w = GetPubVar(Parent(0), "offs_width");
    new Float:extra_h = GetPubVar(Parent(0), "offs_height");
    new Float:move_ms = GetPubVar(Parent(0), "move_time");
    new blinking = GetPubVar(Parent(0), "blink");
    {
        new Float:rot[3];
        GetRot(target, rot[0], rot[1], rot[2], Parent(Parent(0)), 0);
        AnimRotate(Parent(0), rot[0], rot[1], rot[2], move_ms, 0.0, ANIM_FUNC_LINEAR, 0);
    }
    new Float:size[3];
    get_total_scale(target, size[0], size[1], size[2]);
    size[0] += extra_w;
    size[1] += extra_h;
    {
        new Float:half_time = 0.5 * move_ms;
        AnimScale(Parent(0), g_shrunk_size, g_shrunk_size, 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
        AnimColor(Parent(0), 1.0, 1.0, 1.0, g_moving_alpha, half_time, 0.0, ANIM_FUNC_SLOWER);
        target = wait_focused(floatround(half_time, 0));
        if (animation_switch)
        {
            return 0;
        }
        if (!target)
        {
            return -1;
        }
    }
    {
        new Float:half_time = 0.5 * move_ms;
        if (blinking)
        {
            AnimScale(Parent(0), size[0], size[1], 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
            AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.2 * g_base_alpha, half_time, 0.0, ANIM_FUNC_ANGULER);
        }
        else
        {
            AnimScale(Parent(0), size[0], size[1] * g_still_height, 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
            AnimColor(Parent(0), 1.0, 1.0, 1.0, 0.35 * g_base_alpha, half_time, 0.0, ANIM_FUNC_ANGULER);
        }
        target = wait_focused(floatround(half_time, 0));
        if (animation_switch)
        {
            return 0;
        }
        if (!target)
        {
            return -1;
        }
    }
    animation_mode = ANIMATION_STAY;
    animate_stay();
    return 0;
}

/* Same as animate_move, to the size of the text of a label. */
animate_move_label()
{
    new target = GetPubVar(Parent(0), "focused");
    new Float:extra_w = GetPubVar(Parent(0), "offs_width");
    new Float:extra_h = GetPubVar(Parent(0), "offs_height");
    new Float:move_ms = GetPubVar(Parent(0), "move_time");
    new blinking = GetPubVar(Parent(0), "blink");
    {
        new Float:rot[3];
        GetRot(target, rot[0], rot[1], rot[2], Parent(Parent(0)), 0);
        AnimRotate(Parent(0), rot[0], rot[1], rot[2], move_ms, 0.0, ANIM_FUNC_LINEAR, 0);
    }
    new Float:rect[4];
    get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
    rect[2] += extra_w;
    rect[3] += extra_h;
    {
        new Float:half_time = 0.5 * move_ms;
        AnimScale(Parent(0), g_shrunk_size, g_shrunk_size, 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
        AnimColor(Parent(0), 1.0, 1.0, 1.0, g_moving_alpha, half_time, 0.0, ANIM_FUNC_SLOWER);
        target = wait_focused(floatround(half_time, 0));
        if (animation_switch)
        {
            return 0;
        }
        if (!target)
        {
            return -1;
        }
    }
    {
        new Float:half_time = 0.5 * move_ms;
        if (blinking)
        {
            AnimScale(Parent(0), rect[2], rect[3], 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
            AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.2 * g_base_alpha, half_time, 0.0, ANIM_FUNC_ANGULER);
        }
        else
        {
            AnimScale(Parent(0), rect[2], rect[3] * g_still_height, 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
            AnimColor(Parent(0), 1.0, 1.0, 1.0, 0.35 * g_base_alpha, half_time, 0.0, ANIM_FUNC_ANGULER);
        }
        target = wait_focused(floatround(half_time, 0));
        if (animation_switch)
        {
            return 0;
        }
        if (!target)
        {
            return -1;
        }
    }
    animation_mode = ANIMATION_STAY;
    animate_stay();
    return 0;
}

/* Waits ms, or less if the animation is switched or nothing is focused; returns the focused object. */
wait_focused(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames && !animation_switch && GetPubVar(Parent(0), "focused"); i++)
    {
        sleep;
    }
    new target = GetPubVar(Parent(0), "focused");
    return target;
}

/* Gets the scale of obj times its local scale. */
get_total_scale(obj, &Float:x, &Float:y, &Float:z)
{
    new Float:scale[3];
    new Float:local_scale[3];
    GetScale(obj, scale[0], scale[1], scale[2]);
    GetLocalScale(obj, local_scale[0], local_scale[1], local_scale[2]);
    x = scale[0] * local_scale[0];
    y = scale[1] * local_scale[1];
    z = scale[2] * local_scale[2];
}

/* Puts plane on target at once, at zero size or at the size of target, and fades it in. */
place_on_target(from_zero, plane, target, Float:fade_time)
{
    {
        new Float:pos[3];
        get_focus_pos(target, pos, pos[1], pos[2]);
        SetPos(plane, pos[0], pos[1], pos[2], 0);
    }
    {
        new Float:rot[3];
        GetRot(target, rot[0], rot[1], rot[2], Parent(0), 0);
        SetRot(plane, rot[0], rot[1], rot[2], 0, 0);
    }
    if (from_zero)
    {
        SetScale(plane, 0.0, 0.0, 0.0);
    }
    else
    {
        new Float:extra_w = GetPubVar(plane, "offs_width");
        new Float:extra_h = GetPubVar(plane, "offs_height");
        if (class == TAG("LABL"))
        {
            new Float:rect[4];
            get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
            SetScale(plane, rect[2] + extra_w, rect[3] + extra_h, 1.0);
        }
        else
        {
            new Float:size[3];
            get_total_scale(target, size[0], size[1], size[2]);
            SetScale(plane, size[0] + extra_w, size[1] + extra_h, 1.0);
        }
    }
    AnimColor(plane, 1.0, 1.0, 1.0, g_base_alpha, fade_time, 0.0, ANIM_FUNC_LINEAR);
}
