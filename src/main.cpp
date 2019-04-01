#include <mbed.h>
#include <ble/BLE.h>
#include <ThisThread.h>
#include <mbed_events.h>
#include <RedButton.h>

// RX_PIN_NUMBER  = p11,
// TX_PIN_NUMBER  = p9,

static events::EventQueue * q_ptr = NULL;

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext *context)
{
    q_ptr->call(Callback<void()>(&context->ble, &BLE::processEvents));
}

static RedButtonConfig_t params = 
{
    .ble = NULL,
    .q = NULL,
    .adv_interval = 1000,
    .button_pin = BUTTON1,
    .led_pin = LED1
};

int main() {

  q_ptr = mbed_event_queue();
  BLE & ble = BLE::Instance();
  params.ble = &ble;
  params.q = q_ptr;
  ble.onEventsToProcess(scheduleBleEventsProcessing);
  RedButton &red_button = RedButton::getInstance();
  red_button.start(&params);
}