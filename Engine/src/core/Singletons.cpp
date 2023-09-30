#include "Singletons.h"



Singletons* Singletons::Get()
{	
	static Singletons s;
	return &s;
}