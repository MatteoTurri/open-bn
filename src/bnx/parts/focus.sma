/*
 * /opt0/bn/script/parts/focus.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The focus frame of the parts: a 3x3 plane that moves onto the object the "FOC0" message names, takes its size (or
 * the size of its text, for class "LABL") and blinks there; it declares its natives itself instead of including bn.inc.
 */

#pragma dynamic 96

native FrameRate();
native swapchars(c);
native floatcmp(Float:fOne, Float:fTwo);
native Self();
native SetOptionP(obj, option, const Float:data[], cells);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native FindMessage(msg, &a=0, &b=0, &c=0, &d=0);
native AnimScale(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native Destroy(obj);
native RecvMessage(&msg, &a, &b, &Float:c, &d);
native Float:floatmul(Float:oper1, Float:oper2);
native Float:floatadd(Float:dividend, Float:divisor);
native Parent(obj);
native GetOffPos(obj, const Float:x[], &Float:y, &Float:z, Float:off_x, Float:off_y, Float:off_z, ref);
native GetOptionP(obj, option, const data[], cells);
native GetOptionI(obj, option, &value);
native Float:float(value);
native GetScale(obj, &Float:x, &Float:y, &Float:z);
native Float:floatdiv(Float:dividend, Float:divisor);
native IsValidObject(obj);
native AnimModMove(obj, Float:x, Float:y, Float:z);
native GetRot(obj, &Float:x, &Float:y, &Float:z, ref, order);
native AnimModRotate(obj, Float:x, Float:y, Float:z);
native AnimModScale(obj, Float:x, Float:y, Float:z);
native AnimGetState(obj, anim);
native GetPos(obj, &Float:x, &Float:y, &Float:z, ref);
native random(max);
native Float:floatsub(Float:oper1, Float:oper2);
native AnimMoveLine(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native AnimRotate(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, order);
native AnimTimerManager(obj, Float:duration, Float:delay, curve);
native GetLocalScale(obj, &Float:x, &Float:y, &Float:z);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;
native Float:operator/(Float:oper1, Float:oper2) = floatdiv;
native Float:operator+(Float:oper1, Float:oper2) = floatadd;
native Float:operator-(Float:oper1, Float:oper2) = floatsub;

forward Float:float_abs(Float:value);

/* As in bn.inc. */
const ANIM_FUNC_LINEAR = 1;
const ANIM_FUNC_SLOWER = 2;
const ANIM_FUNC_FASTER = 3;
const ANIM_FUNC_ANGULER = 5;
const ANIM_COLOR = 3;
const ANIM_TIMER = 4;
const ANIM_DONE = 0;
const ALIGN_LEFT = 0;
const ALIGN_CENTER = 1;

/* Values of g_mode. */
enum
{
    ANIMATION_STAY,   /* stays on the focused object */
    ANIMATION_HIDDEN, /* nothing focused: shrunk, drifting */
    ANIMATION_MOVE    /* moves onto the focused object, then stays */
};

new g_focused; /* the object to frame, 0 for none */
new g_class;   /* "LABL" to frame the text of a label */
new Float:g_zoffs;
new Float:g_sharpness = 0.5;   /* margin of the corners of the frame */
new Float:g_offs_width = 60.0; /* added to the size of the object */
new Float:g_offs_height = 60.0;
new g_smooth = 1; /* animate the moves; without it the frame jumps */
new Float:g_move_time = 150.0;
new g_blink = 1;
new Float:g_shrunk_size = 50.0; /* size of the frame halfway through a move */
new Float:g_base_alpha = 0.4;
new Float:g_hidden_alpha;
new Float:g_moving_alpha;
new Float:g_still_height = 0.7; /* height ratio of a frame that does not blink */
new g_mode = ANIMATION_HIDDEN;
new g_last_mode = -1; /* g_mode at the last pass, -1 before the first */

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
    for (; ; )
    {
        recv_params();
        if (g_focused)
        {
            break;
        }
        sleep;
    }
    follow_target();
    g_mode = ANIMATION_STAY;
    sleep;
    for (new last_focused = -1; ; )
    {
        if (FindMessage(TAG("KILL")))
        {
            /* the frame shrinks, fades out and destroys itself */
            AnimScale(Self(), 10.0, 10.0, 10.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, 1200.0, 0.0, ANIM_FUNC_ANGULER);
            wait_ms(1200);
            Destroy(Self());
        }
        recv_params();
        if (g_focused != last_focused)
        {
            if (g_focused)
            {
                g_mode = ANIMATION_MOVE;
            }
            else
            {
                g_mode = ANIMATION_HIDDEN;
            }
        }
        last_focused = g_focused;
        run_animation();
        follow_target();
        update_margin();
        sleep;
    }
}

/*
 * Takes the parameters of the frame from the messages "FOC0" (target, class, zoffs, sharpness), "FOC1" (offs_width,
 * offs_height, move_time, smooth) and "FOC2" (blink).
 */
recv_params()
{
    new msg;
    new a;
    new b;
    new Float:c;
    new d;
    while (RecvMessage(msg, a, b, c, d))
    {
        if (msg == TAG("FOC0"))
        {
            g_focused = a;
            g_class = b;
            g_zoffs = c;
            g_sharpness = d;
        }
        if (msg == TAG("FOC1"))
        {
            g_offs_width = a;
            g_offs_height = b;
            g_move_time = c;
            g_smooth = d;
        }
        if (msg == TAG("FOC2"))
        {
            g_blink = a;
        }
    }
}

Float:float_abs(Float:value)
{
    return value > 0.0 ? value : -value;
}

/* Gets the size of target, or of its text for target_class "LABL". */
get_target_size(target, target_class, &Float:w, &Float:h, &Float:d)
{
    if (target_class == TAG("LABL"))
    {
        new Float:text_pos[2];
        get_text_rect(target, text_pos[0], text_pos[1], w, h);
        d = 1.0;
    }
    else
    {
        get_total_scale(target, w, h, d);
    }
}

/* Gets where the frame goes on target, in the parent: its origin, or the center of the text of a label. */
get_focus_pos(target, Float:pos[], &Float:y, &Float:z)
{
    if (g_class != TAG(""))
    {
        new Float:rect[4];
        get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
        if (g_class == TAG("LABL"))
        {
            GetOffPos(target, pos, y, z, rect[0] + 0.5 * rect[2], -float_abs(rect[1]) + 0.5 * rect[3], g_zoffs,
                    Parent(0));
        }
    }
    else
    {
        GetOffPos(target, pos, y, z, 0.0, 0.0, g_zoffs, Parent(0));
    }
}

/* Gets the rectangle of the text of obj, cut to its "W64 " width limit, scaled like obj. */
get_text_rect(obj, &Float:x, &Float:y, &Float:w, &Float:h)
{
    new rect[5];
    GetOptionP(obj, TAG("RECT"), rect, 5);
    if (!rect[0])
    {
        x = 0.0;
        y = 0.0;
        w = 0.0;
        h = 0.0;
        return;
    }
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
        x = 0.015625 * float(rect[1]);
        y = 0.015625 * float(rect[2]);
        w = 0.015625 * float(rect[3]);
        h = 0.015625 * float(rect[4]);
    }
    new Float:scale[3];
    get_total_scale(obj, scale[0], scale[1], scale[2]);
    x = x * scale[0];
    y = y * scale[1];
    w = w * scale[0];
    h = h * scale[1];
}

/* Keeps the corners of the frame round while it is resized. */
update_margin()
{
    new Float:scale[3];
    GetScale(Self(), scale[0], scale[1], scale[2]);
    new Float:frame_margin[4];
    if (scale[0] > scale[1])
    {
        frame_margin[0] = frame_margin[1] = g_sharpness * scale[1] / scale[0];
        frame_margin[2] = frame_margin[3] = g_sharpness;
    }
    else
    {
        frame_margin[2] = frame_margin[3] = g_sharpness * scale[0] / scale[1];
        frame_margin[0] = frame_margin[1] = g_sharpness;
    }
    SetOptionP(Self(), TAG("MGN"), frame_margin, 4);
}

new g_brighten = 1; /* the next blink brightens the frame */
new Float:g_drift_pos[3];
new Float:g_drift_home[3];
new Float:g_drift_velocity[3];
/* Values of g_move_phase. */
enum
{
    MOVE_SHRINKING, /* on the way */
    MOVE_GROWING    /* on the object */
};

new g_move_phase;

/* Keeps the frame on the focused object and, while it stays there, at its size. */
follow_target()
{
    if (IsValidObject(g_focused))
    {
        new Float:vec[3];
        get_focus_pos(g_focused, vec, vec[1], vec[2]);
        AnimModMove(Self(), vec[0], vec[1], vec[2]);
        GetRot(g_focused, vec[0], vec[1], vec[2], Parent(0), 0);
        AnimModRotate(Self(), vec[0], vec[1], vec[2]);
        if (ANIMATION_STAY == g_mode)
        {
            get_target_size(g_focused, g_class, vec[0], vec[1], vec[2]);
            if (g_blink)
            {
                vec[0] += g_offs_width;
                vec[1] += g_offs_height;
            }
            else
            {
                vec[0] += g_offs_width;
                vec[1] = (vec[1] + g_offs_height) * g_still_height;
            }
            AnimModScale(Self(), vec[0], vec[1], 1.0);
        }
    }
}

/* Runs a frame of the animation of g_mode, starting it when the mode changes. */
run_animation()
{
    if (!IsValidObject(g_focused))
    {
        if (ANIMATION_HIDDEN != g_last_mode)
        {
            start_hidden();
            g_mode = ANIMATION_HIDDEN;
        }
        animate_hidden();
    }
    else
    {
        if (!g_smooth)
        {
            g_move_time = 0.0;
            g_mode = ANIMATION_STAY;
        }
        if (g_last_mode != g_mode)
        {
            switch (g_mode)
            {
                case ANIMATION_STAY:
                {
                    start_stay();
                }
                case ANIMATION_HIDDEN:
                {
                    start_hidden();
                }
                case ANIMATION_MOVE:
                {
                    start_move();
                }
            }
        }
        switch (g_mode)
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
        }
    }
    g_last_mode = g_mode;
}

start_stay()
{
    g_brighten = 0;
}

/* Blinks the frame, a color animation at a time. */
animate_stay()
{
    if (ANIM_DONE == AnimGetState(Self(), ANIM_COLOR) && g_blink)
    {
        if (g_brighten)
        {
            g_brighten = 0;
            AnimColor(Self(), 1.0, 1.0, 1.0, 0.8 * g_base_alpha, 1200.0, 0.0, ANIM_FUNC_ANGULER);
        }
        else
        {
            g_brighten = 1;
            AnimColor(Self(), 1.0, 1.0, 1.0, 1.2 * g_base_alpha, 1200.0, 0.0, ANIM_FUNC_ANGULER);
        }
    }
}

/* Shrinks the frame. */
start_hidden()
{
    AnimScale(Self(), 20.0, 20.0, 20.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(Self(), 1.0, 1.0, 1.0, g_hidden_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
}

/* Moves the shrunk frame by a random step. */
animate_hidden()
{
    GetPos(Self(), g_drift_pos[0], g_drift_pos[1], g_drift_pos[2], 0);
    /* NOTE: bug: home and velocity are set anew on every frame, so the frame drifts away instead of around */
    g_drift_home[0] = g_drift_pos[0];
    g_drift_home[1] = g_drift_pos[1];
    g_drift_home[2] = g_drift_pos[2];
    g_drift_velocity[0] = 0.1 * float(random(15));
    g_drift_velocity[1] = 0.1 * float(random(15));
    g_drift_velocity[2] = 0.1 * float(random(15));
    if (g_drift_home[0] > g_drift_pos[0])
    {
        g_drift_velocity[0] += 0.01;
    }
    else
    {
        g_drift_velocity[0] -= 0.01;
    }
    if (g_drift_home[1] > g_drift_pos[1])
    {
        g_drift_velocity[1] += 0.01;
    }
    else
    {
        g_drift_velocity[1] -= 0.01;
    }
    if (g_drift_home[2] > g_drift_pos[2])
    {
        g_drift_velocity[2] += 0.01;
    }
    else
    {
        g_drift_velocity[2] -= 0.01;
    }
    g_drift_pos[0] += g_drift_velocity[0];
    g_drift_pos[1] += g_drift_velocity[1];
    g_drift_pos[2] += g_drift_velocity[2];
    AnimMoveLine(Self(), g_drift_pos[0], g_drift_pos[1], g_drift_pos[2], 100.0, 0.0, ANIM_FUNC_LINEAR);
}

/* Sends the frame onto the focused object, shrinking it for the first half of the move. */
start_move()
{
    new Float:vec[3];
    get_focus_pos(g_focused, vec, vec[1], vec[2]);
    AnimMoveHermite(Self(), vec[0], vec[1], vec[2], g_move_time, 0.0, ANIM_FUNC_ANGULER, 0.2);
    GetRot(g_focused, vec[0], vec[1], vec[2], Parent(0), 0);
    AnimRotate(Self(), vec[0], vec[1], vec[2], g_move_time, 0.0, ANIM_FUNC_LINEAR, 0);
    {
        new Float:half_time = 0.5 * g_move_time;
        AnimScale(Self(), g_shrunk_size, g_shrunk_size, 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
        AnimColor(Self(), 1.0, 1.0, 1.0, g_moving_alpha, half_time, 0.0, ANIM_FUNC_SLOWER);
        AnimTimerManager(Self(), half_time, 0.0, ANIM_FUNC_LINEAR);
        g_move_phase = MOVE_SHRINKING;
    }
}

/* At half the move grows the frame to the size of the object; at the end it stays there. */
animate_move()
{
    if (ANIM_DONE == AnimGetState(Self(), ANIM_TIMER))
    {
        if (MOVE_SHRINKING == g_move_phase)
        {
            new Float:size[3];
            get_target_size(g_focused, g_class, size[0], size[1], size[2]);
            size[0] += g_offs_width;
            size[1] += g_offs_height;
            new Float:half_time = 0.5 * g_move_time;
            if (g_blink)
            {
                AnimScale(Self(), size[0], size[1], 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
                AnimColor(Self(), 1.0, 1.0, 1.0, 1.2 * g_base_alpha, half_time, 0.0, ANIM_FUNC_FASTER);
            }
            else
            {
                AnimScale(Self(), size[0], size[1] * g_still_height, 1.0, half_time, 0.0, ANIM_FUNC_ANGULER);
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.35 * g_base_alpha, half_time, 0.0, ANIM_FUNC_FASTER);
            }
            AnimTimerManager(Self(), half_time, 0.0, ANIM_FUNC_LINEAR);
            g_move_phase = MOVE_GROWING;
        }
        else if (g_move_phase == MOVE_GROWING)
        {
            g_mode = ANIMATION_STAY;
        }
    }
}

/* Gets the scale of obj times its local scale. */
get_total_scale(obj, &Float:x, &Float:y, &Float:z)
{
    new Float:local_scale[3];
    GetScale(obj, x, y, z);
    GetLocalScale(obj, local_scale[0], local_scale[1], local_scale[2]);
    x = x * local_scale[0];
    y = y * local_scale[1];
    z = z * local_scale[2];
}
