#include "RedButtonService.h"
#include "ble_debug_logs.h"

const UUID RedButtonServiceUUID("1b62cd63-99ae-4345-9b2d-9ba8247a6de3");
const UUID RedButtonCharacteristic1UUID("1b62cd64-99ae-4345-9b2d-9ba8247a6de3");

void RedButtonService::dataReadCallback(const GattReadCallbackParams *context)
{
    if(context->handle == _rb_char.getValueHandle())
    {
        BLE_LOG("Red Button Characteristic was read!\r\n");
    }
}

void RedButtonService::updatesEnabledCallback(GattAttribute::Handle_t handle)
{
    _notification_enabled = true;
    BLE_LOG("Red Button Characteristic notification enabled\r\n");
}

void RedButtonService::updatesDisabledCallback(GattAttribute::Handle_t handle)
{
    _notification_enabled = false;
    BLE_LOG("Red Button Characteristic notification disabled\r\n");
}

int RedButtonService::update(uint8_t value)
{
    ble_error_t err = _server.write(_rb_char.getValueHandle(), &value, 1);
    if(err == BLE_ERROR_NONE)     
        return 0;
    else
        return 1;
}  
 