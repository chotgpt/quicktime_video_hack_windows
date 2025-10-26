#include "config_msc.h"

char *dirname(char const *file)
{
	static char dir[260];
	_splitpath(file, 0, dir, 0, 0);
	return dir;
}