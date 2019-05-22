#ifndef __inotify__handlers__
#define __inotify__handlers__

#include <stdio.h>

void handlerSIGCHLDmanager(int sigNo);
void handlerSIGINTmanager(int sigNo);
void handlerSIGUSR1worker(int sigNo);
void handlerSIGUSR1listener(int sigNo);
void handlerSIGINTworker(int sigNo);
void handlerSIGINTlistener(int sigNo);

#endif /* defined(__inotify__handlers__) */
