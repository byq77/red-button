#ifndef __RED_BUTTON_H__
#define __RED_BUTTON_H__
#include <mbed.h>
#include <ble/BLE.h>
#include <mbed_events.h>
#include "RedButtonService.h"

typedef struct RedButtonConfig
{
  BLE * ble;
  events::EventQueue * q;
  uint16_t adv_interval;
  PinName button_pin;
  PinName led_pin;
}RedButtonConfig_t;

class RedButton
{
  public:
    /**
     * @brief RedButton initialisation.
     *
     * This function have to be called after obtaining the class instance.
     */
    void start(const RedButtonConfig_t * config);
    static RedButton& getInstance();
    void printMacAddress();

  private:
    RedButton()
    : _ble(NULL), _q(NULL), _is_connected(false), _adv_interval(500),
    _button(NULL), _led(NULL)
    {}

    template<typename Arg>
    FunctionPointerWithContext<Arg> as_cb(void (RedButton::*member)(Arg))
    {
        return makeFunctionPointer(this, member);
    }

    void bleInit(BLE::InitializationCompleteCallbackContext *context);
    void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params);
    void connectionCallback(const Gap::ConnectionCallbackParams_t *params);
    const char * debugDisconnectionReason(Gap::DisconnectionReason_t reason); 
    void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext *context);
    void signalEvent();
    void updateButtonState(bool state);
    void printMSD();
    void updateButtonPressed();
    void updateButtonReleased();
    BLE * _ble;
    EventQueue *_q;
    bool _is_connected;
    uint16_t _adv_interval;
    DigitalOut * _led;
    InterruptIn * _button;

    volatile bool _update_in_progress;
    RedButtonService * _rb_service_ptr;
    Gap::Handle_t _connection_handle;
    static const char DEVICE_NAME[]; ///< Device brodcast name
    static const uint8_t MSD[]; ///< Manufacturer Specific Data
    static const Gap::ConnectionParams_t PREFFERED_CONNECTION_PARAMS;
    static const Gap::ConnectionParams_t CONNECTION_PARAMS;
};

#endif /* __RED_BUTTON_H__ */

