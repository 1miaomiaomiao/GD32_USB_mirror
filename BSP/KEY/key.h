#ifndef __KEY_H__
#define __KEY_H__

#include <rtthread.h>

void key_exti_init(void);

void key_thread_entry(void *parameter);

#endif
