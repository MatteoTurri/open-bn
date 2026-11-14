/*
 * BN: the Broadband Navigator application object. Destroying it stops every
 * worker thread.
 */

#include "thread/thposixthread.h"
#include "bn.h"
#include "systemoptions.h"
#include "variables.h"

BN::BN(int a, int b) : Desktop(a, b)
{}

BN::~BN()
{
	thPosixThread::AllDelete();
}
