#ifndef LIBCAPTURE_MUTEX_H
#define LIBCAPTURE_MUTEX_H

#include <freertos/semphr.h>

void lock_mutex(SemaphoreHandle_t l)
{
    while (xSemaphoreTake(l, (TickType_t)10) != pdTRUE)
    {
    }
    return;
}

void unlock_mutex(SemaphoreHandle_t l)
{
    xSemaphoreGive(l);
}

#endif // LIBCAPTURE_MUTEX_H