#ifndef __inotify__parser__
#define __inotify__parser__

#include <stdio.h>

TLDPtr parser(char *path, char *fileName);
void writer(TLDPtr listStart, char *fileName);

#endif /* defined(__inotify__parser__) */
