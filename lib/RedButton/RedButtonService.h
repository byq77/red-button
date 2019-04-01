#ifndef __RED_BUTTON_SERVICE_H__
#define __RED_BUTTON_SERVICE_H__

#include <mbed.h>
#include <ble/BLE.h>
#include <UUID.h>
#include <mbed_events.h>

extern const UUID RedButtonServiceUUID;
extern const UUID RedButtonCharacteristic1UUID;

class RedButtonService
{
    typedef RedButtonService Self;

public:

    RedButtonService(BLE &ble, uint8_t default_init_value)
    :_ble(ble)
    ,_rb_char(RedButtonCharacteristic1UUID, default_init_value)
    ,_rb_service(
    /* uuid */ RedButtonServiceUUID,
    /* characteristics */ _rb_chars,
    /* numCharacteristics */ sizeof(_rb_chars) /
                             sizeof(_rb_chars[0]))
    ,_server(_ble.gattServer())
    ,_notification_enabled(false)
    {
        // update internal pointers (value, descriptors and characteristics array)
        _rb_chars[0] = &_rb_char;
        _server.addService(_rb_service);
        if(_server.isOnDataReadAvailable())
            _server.onDataRead(as_cb(&Self::dataReadCallback));
        _server.onUpdatesEnabled(as_cb(&Self::updatesEnabledCallback));
        _server.onUpdatesDisabled(as_cb(&Self::updatesDisabledCallback));
    }

    int update(uint8_t value);
    bool isNotificationEnabled(){return _notification_enabled;}
    
private:
     /**
     * Helper that construct an event handler from a member function of this
     * instance.
     */
    template<typename Arg>
    FunctionPointerWithContext<Arg> as_cb(void (Self::*member)(Arg))
    {
        return makeFunctionPointer(this, member);
    }

    // void dataWrittenCallback(const GattWriteCallbackParams *params);
    void dataReadCallback(const GattReadCallbackParams *context);
    void updatesEnabledCallback(GattAttribute::Handle_t handle);
    void updatesDisabledCallback(GattAttribute::Handle_t handle);
    /**
     * Write Without Response Characteristic declaration helper.
     *
     * @tparam T type of data held by the characteristic.
     */
    template<typename T>
    class ReadAndNotifyCharacteristic : public GattCharacteristic {
    public:
        /**
         * Construct a characteristic that can be written.
         *
         * @param[in] uuid The UUID of the characteristic.
         * @param[in] initial_value Initial value contained by the characteristic.
         */
        ReadAndNotifyCharacteristic<T>(const UUID & uuid, const T initial_value) :
            GattCharacteristic(
                /* UUID */ uuid,
                /* Initial value */ reinterpret_cast<uint8_t *>(&_value),
                /* Value size */ sizeof(T),
                /* Value capacity */ sizeof(T),
                /* Properties */ GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
                /* Descriptors */ NULL,
                /* Num descriptors */ 0,
                /* variable len */ false
            ),
            _value(initial_value) {
        }

        /**
         * Get the value of this characteristic.
         *
         * @param[in] server GattServer instance that contain the characteristic
         * value.
         * @param[in] dst Variable that will receive the characteristic value.
         *
         * @return BLE_ERROR_NONE in case of success or an appropriate error code.
         */
        ble_error_t get(GattServer &server, T& dst) const
        {
            uint16_t value_length = sizeof(dst);
            return server.read(getValueHandle(), (uint8_t *)&dst, &value_length);
        }

        /**
         * Assign a new value to this characteristic.
         *
         * @param[in] server GattServer instance that will receive the new value.
         * @param[in] value The new value to set.
         * @param[in] local_only Flag that determine if the change should be kept
         * locally or forwarded to subscribed clients.
         */
        ble_error_t set(
            GattServer &server, const T &value, bool local_only = false
        ) const {
            return server.write(getValueHandle(), (uint8_t *)&value, sizeof(value), local_only);
        }

        /**
         * Get initial value of this characteristic
         */
        T getInitialValue()
        {
            return _value;
        }

    private:
        T _value;
    };

    BLE & _ble;
    ReadAndNotifyCharacteristic<uint8_t> _rb_char;
    GattCharacteristic* _rb_chars[1];
    GattService _rb_service;
    GattServer & _server;
    volatile bool _notification_enabled;
};

#endif /* __RED_BUTTON_SERVICE_H__ */