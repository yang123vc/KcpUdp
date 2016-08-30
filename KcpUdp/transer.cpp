#include "transer.h"

void* Work(void* pVoid)
{
	transer* pThis = (transer*)pVoid;
	pThis->DoWork();
}