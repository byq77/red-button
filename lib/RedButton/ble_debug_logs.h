#ifndef __BLE_DEBUG_LOGS_H__
#define __BLE_DEBUG_LOGS_H__
#if DEBUG_BLUETOOTH
    #define BLE_LOG(f_ , ...) printf((f_), ##__VA_ARGS__)
#else
    #define BLE_LOG(f_ , ...) do {} while (0)
#endif
#endif