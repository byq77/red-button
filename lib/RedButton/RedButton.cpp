#include "RedButton.h"
#include <DeviceInformationService.h>
#include <UUID.h>
#include "ble_debug_logs.h"

const char RedButton::DEVICE_NAME[]= "RED-BUTTON";
const uint8_t RedButton::MSD[] = {'H','U'};
const Gap::ConnectionParams_t RedButton::PREFFERED_CONNECTION_PARAMS = {16, 24, 0, 400}; 
const Gap::ConnectionParams_t RedButton::CONNECTION_PARAMS = {16, 64, 0, 400};
static Ticker led_flipper;

RedButton& RedButton::getInstance()
{
    static RedButton instance;
    return instance;
}

void RedButton::start(const RedButtonConfig_t * config)
{
    static bool initialised = false;
    if(initialised)
        return;
    _ble = config->ble;
    _q = config->q;
    _adv_interval = config->adv_interval;

    _button = new InterruptIn(config->button_pin);
    _led = new DigitalOut(config->led_pin,0);

    led_flipper.attach(callback(this,&RedButton::signalEvent),1.0);
    _update_in_progress = false;
    
    _button->mode(PullUp);
    _button->fall(callback(this,&RedButton::updateButtonPressed));
    _button->rise(callback(this,&RedButton::updateButtonReleased));
    
    _ble->init(this, &RedButton::bleInit);
    initialised = true;

    _q->dispatch_forever();
}

void RedButton::signalEvent()
{
    static bool state = false;
    _led->write(state=!state);
}

void RedButton::updateButtonPressed()
{
    if(_update_in_progress)
        return;
    else
        _update_in_progress = true;
    _q->call(callback(this,&RedButton::updateButtonState),false);
}

void RedButton::updateButtonReleased()
{
    if(_update_in_progress)
        return;
    else
        _update_in_progress = true;
    _q->call(callback(this,&RedButton::updateButtonState),true);
}

void RedButton::updateButtonState(bool state)
{
    wait_ms(10);
    if(state != _button->read())
        return;
    if(state)
    {
        if(_is_connected)
        {
            led_flipper.detach();
            _led->write(1);
        }
        _rb_service_ptr->update(0);
        BLE_LOG("Button released!\r\n");
    }
    else
    {
        if(_is_connected)
        {
            if(_rb_service_ptr->update(1) == 0 && _rb_service_ptr->isNotificationEnabled())
                led_flipper.attach(callback(this,&RedButton::signalEvent),0.1);
        }
        else
        {
            _rb_service_ptr->update(1);
        }
        BLE_LOG("Button pushed!\r\n");
    }
    _update_in_progress = false;
}

void RedButton::bleInit(BLE::InitializationCompleteCallbackContext *context)
{
    ble_error_t &err = context->error;
    BLE &ble = context->ble;
    
    BLE_LOG("RedButton initialisation started!\r\n");

    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Initialisation failed: %s\r\n", BLE::errorToString(err));
        MBED_ASSERT(0);
    }

    #pragma region gatt_setup

    DeviceInformationService(ble,"Husarion");
    // ble.gattServer().onDataWritten(this, &RedButton::onDataWrittenCallback);
    bool initial_value = (bool)_button->read();
    _rb_service_ptr = new RedButtonService(ble,!initial_value);


    #pragma endregion /* gatt_setup */

    #pragma region gap_setup
    ble.gap().onDisconnection(this, &RedButton::disconnectionCallback);

    ble.gap().onConnection(this, &RedButton::connectionCallback);
    
    err = ble.gap().setDeviceName((const uint8_t*)RedButton::DEVICE_NAME);
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Setting device name failed: %s\r\n", BLE::errorToString(err));
    }

    err = ble.gap().setAppearance(GapAdvertisingData::UNKNOWN);
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Setting Apperance failed: %s\r\n", BLE::errorToString(err));
    }

    err = ble.gap().setPreferredConnectionParams(&PREFFERED_CONNECTION_PARAMS);
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Setting Preffered Connection Params failed: %s\r\n",BLE::errorToString(err));
    }

    // Set up the advertising flags. Note: not all combination of flags are valid
    // BREDR_NOT_SUPPORTED: Device does not support Basic Rate or Enchanced Data Rate, It is Low Energy only.
    // LE_GENERAL_DISCOVERABLE: Peripheral device is discoverable at any moment
    err = ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Setting GAP flags failed: %s\r\n", BLE::errorToString(err));
    }
    
    err = ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::INCOMPLETE_LIST_128BIT_SERVICE_IDS, RedButtonServiceUUID.getBaseUUID(), 16);
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Setting 128bit uuid in payload failed: %s\r\n", BLE::errorToString(err));
    }

    ble.gap().clearScanResponse();
    
    // // Manufacturer Specific Data
    // err = ble.gap().accumulateScanResponse(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, MSD, sizeof(MSD));
    // if (err != BLE_ERROR_NONE)
    // {
    //     BLE_LOG("Setting device manufacturer specific data failed: %s\r\n", BLE::errorToString(err));
    // }

    // Put the device name in the advertising payload
    err = ble.gap().accumulateScanResponse(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)RedButton::DEVICE_NAME, sizeof(RedButton::DEVICE_NAME));
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Setting device name failed: %s\r\n", BLE::errorToString(err));
    }

    // ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_SCANNABLE_UNDIRECTED); // becon mode
    
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED); // connectable
    
    // Send out the advertising payload at interval
    ble.gap().setAdvertisingInterval(_adv_interval);

    // printMSD();
    printMacAddress();

    err = ble.gap().startAdvertising();
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Start advertising failed: %s\r\n",BLE::errorToString(err));
    }

    BLE_LOG("RedButton initialisation completed!\r\n");
    #pragma endregion /* gap_setup */
}

void RedButton::disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE_LOG("Device disconnected!\r\n");
    BLE_LOG("Disconnection reason: %s\r\n",debugDisconnectionReason(params->reason));
    _is_connected = false;
    led_flipper.detach();
    led_flipper.attach(callback(this,&RedButton::signalEvent),1.0);
    _ble->gap().startAdvertising();
}

void RedButton::connectionCallback(const Gap::ConnectionCallbackParams_t *params)
{
    BLE_LOG("Device connected!\r\n");
    _connection_handle = params->handle;
    _is_connected = true;
    led_flipper.detach();
    _led->write(1);
    ble_error_t err = _ble->gap().updateConnectionParams(_connection_handle, &CONNECTION_PARAMS);
    if (err != BLE_ERROR_NONE) {
        BLE_LOG("Updating connection params failed: %s\r\n",BLE::errorToString(err));
    }
}

const char * RedButton::debugDisconnectionReason(Gap::DisconnectionReason_t reason)
{
    switch (reason)
    {
    case Gap::CONNECTION_TIMEOUT:
        return "CONNECTION_TIMEOUT";
    case Gap::REMOTE_USER_TERMINATED_CONNECTION:
        return "REMOTE_USER_TERMINATED_CONNECTION";
    case Gap::REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES:
        return "REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES";
    case Gap::REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF:
        return "REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF";
    case Gap::LOCAL_HOST_TERMINATED_CONNECTION:
        return "LOCAL_HOST_TERMINATED_CONNECTION";
    case Gap::CONN_INTERVAL_UNACCEPTABLE:
        return "CONN_INTERVAL_UNACCEPTABLE";
    default:
        return "Illegal error code!";
    }
}

void RedButton::printMSD()
{
    BLE_LOG("MSD: 0x",MSD[0],MSD[1]);
    for(int i=0; i<sizeof(RedButton::MSD); i++)
    {
        BLE_LOG("%02X",MSD[i]);
    }
    BLE_LOG("\r\n");
}

void RedButton::printMacAddress()
{
    if(!_ble->hasInitialized())
        return;
    /* Print out device MAC address to the console*/
    Gap::AddressType_t addr_type;
    Gap::Address_t address;
    _ble->gap().getAddress(&addr_type, address);
    BLE_LOG("DEVICE MAC ADDRESS: ");
    for (int i = 5; i >= 1; i--){
        BLE_LOG("%02x:", address[i]);
    }
    BLE_LOG("%02x\r\n", address[0]);
}