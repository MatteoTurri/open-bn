/*
 * NaviMenu: the navigation menu window (load, open, enable and lock).
 */

#ifndef NAVIMENU_H
#define NAVIMENU_H

namespace NaviMenu {
void Init();
void Load();
void Exit();
int Open();
void Enable(bool enable);
void Lock(bool lock);
}

#endif
