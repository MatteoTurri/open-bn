/*
 * Error: the pending fatal or script error and the PSML panel that shows it.
 */

#ifndef SCRIPTABORT_H
#define SCRIPTABORT_H

namespace Error {
void Init();
void Exit();
void Run();
void Raise(int code, int arg1, int arg2);
}

#endif
