#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/drivers/sensor/mcp9600.h>
#include <zephyr/logging/log.h>

#include <ram_pwrdn.h>
#include <dk_buttons_and_leds.h>

#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zb_nrf_platform.h>
#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>

#include <stdlib.h>

#include "thermocouple/thermocouple.h"

/* Bits of the nPM1300 charge status register (BCHGCHARGESTATUS) */
#define CHARGER_STATUS_COMPLETE BIT(1)
#define CHARGER_STATUS_TRICKLE  BIT(2)
#define CHARGER_STATUS_CC       BIT(3)
#define CHARGER_STATUS_CV       BIT(4)

/* Interval for the temperature measurement. The interrupt pin of the nPM1300 can not be used,
 * so the battery is polled too (see CONFIG_THERMOCOUPLE_BATTERY_INTERVAL_S).
 */
#define MEASUREMENT_INTERVAL_MS (CONFIG_THERMOCOUPLE_MEASUREMENT_INTERVAL_S * 1000UL)

/* MCP9600 registers and bits for the power saving burst mode */
#define MCP9600_REG_STATUS            0x04
#define MCP9600_REG_DEV_CONFIG        0x06
#define MCP9600_STATUS_BURST_COMPLETE BIT(7)
#define MCP9600_MODE_MASK             (BIT_MASK(2) << 0)
#define MCP9600_MODE_SHUTDOWN         0x01
#define MCP9600_MODE_BURST            0x02

/* One conversion with 18 bit resolution takes approx. 320 ms */
#define MCP9600_BURST_POLL_MS    50
#define MCP9600_BURST_TIMEOUT_MS 1500

/* Interval for the temperature output in debug builds. Independent of the Zigbee connection. */
#define DEBUG_PRINT_INTERVAL_MS 10000

static const struct device *const mcp9600 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(mcp9600));
static const struct device *const npm1300 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm1300));
static const struct device *const leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_leds));
static const struct device *const charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));

/** @brief Zigbee device application context storage.
 */
static device_ctx_t dev_ctx;

/* The MCP9600 is read by the Zigbee thread and by the debug output, so fetch and read
 * must not be interrupted by the other context.
 */
static K_MUTEX_DEFINE(mcp9600_lock);

/* Direct register access to the MCP9600 for the power modes and the diagnostics */
static const struct i2c_dt_spec mcp9600_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(mcp9600));

static void debug_print_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(debug_print_work, debug_print_handler);

/* Declare attribute list for Identify cluster (client). */
ZB_ZCL_DECLARE_IDENTIFY_CLIENT_ATTRIB_LIST(identify_client_attr_list);

/* Declare attribute list for Identify cluster (server). */
ZB_ZCL_DECLARE_IDENTIFY_SERVER_ATTRIB_LIST(identify_server_attr_list,
                                           &dev_ctx.identify_attr.identify_time);

/* Declare attribute list for Basic cluster (server). */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
    basic_attr_list, &dev_ctx.basic_attr.zcl_version, &dev_ctx.basic_attr.app_version,
    &dev_ctx.basic_attr.stack_version, &dev_ctx.basic_attr.hw_version, dev_ctx.basic_attr.mf_name,
    dev_ctx.basic_attr.model_id, dev_ctx.basic_attr.date_code, &dev_ctx.basic_attr.power_source,
    dev_ctx.basic_attr.location_id, &dev_ctx.basic_attr.ph_env, dev_ctx.basic_attr.sw_ver);

/* Declare attribute list for Power Configuration cluster (server). */
ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT(
    power_attr_list, &dev_ctx.power_attr.voltage, &dev_ctx.power_attr.size,
    &dev_ctx.power_attr.quantity, &dev_ctx.power_attr.rated_voltage, &dev_ctx.power_attr.alarm_mask,
    &dev_ctx.power_attr.voltage_min_threshold, &dev_ctx.power_attr.percent_remaining,
    &dev_ctx.power_attr.voltage_threshold_1, &dev_ctx.power_attr.voltage_threshold_2,
    &dev_ctx.power_attr.voltage_threshold_3, &dev_ctx.power_attr.percent_min_threshold,
    &dev_ctx.power_attr.percent_threshold_1, &dev_ctx.power_attr.percent_threshold_2,
    &dev_ctx.power_attr.percent_threshold_3, &dev_ctx.power_attr.alarm_state);

/* Declare attribute list for Temperature Measurement cluster (server). */
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_sensor_attr_list,
                                            &dev_ctx.temperature_attr.measurement_attr,
                                            &dev_ctx.temperature_attr.min_attr,
                                            &dev_ctx.temperature_attr.max_attr,
                                            &dev_ctx.temperature_attr.tolerance);

THERMOCOUPLE_DECLARE_CLUSTER_LIST(thermocouple_clusters, basic_attr_list, identify_client_attr_list,
                                  identify_server_attr_list, power_attr_list,
                                  temp_sensor_attr_list);

THERMOCOUPLE_DECLARE_EP(thermocouple_ep, THERMOCOUPLE_ENDPOINT, thermocouple_clusters);

ZBOSS_DECLARE_DEVICE_CTX_1_EP(thermocouple_ctx, thermocouple_ep);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/** @brief          Function to toggle the identify LED.
 *  @param bufid    Unused parameter, required by ZBOSS scheduler API
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
    static bool is_on;

    is_on = !is_on;
    if (is_on) {
        led_on(leds, ZIGBEE_NETWORK_STATE_LED);
    } else {
        led_off(leds, ZIGBEE_NETWORK_STATE_LED);
    }

    ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/** @brief          Starts identifying the device.
 *  @param bufid    Unused parameter, required by ZBOSS scheduler API
 */
static void start_identifying(zb_bufid_t bufid)
{
    ZVUNUSED(bufid);

    if (ZB_JOINED()) {
        if (dev_ctx.identify_attr.identify_time == ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {

            zb_ret_t zb_err_code = zb_bdb_finding_binding_target(THERMOCOUPLE_ENDPOINT);

            if (zb_err_code == RET_OK) {
                LOG_INF("Enter identify mode");
            } else if (zb_err_code == RET_INVALID_STATE) {
                LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
            } else {
                ZB_ERROR_CHECK(zb_err_code);
            }
        } else {
            LOG_INF("Cancel identify mode");
            zb_bdb_finding_binding_target_cancel();
        }
    } else {
        LOG_WRN("Device not in a network - cannot enter identify mode");
    }
}

static void on_Identify_Handler(zb_bufid_t bufid)
{
    zb_ret_t err = RET_OK;

    if (bufid) {
        /* Schedule a self-scheduling function that will toggle the LED */
        err = ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
        if (err) {
            LOG_ERR("Failed to schedule app callback: %d", err);
        } else {
            LOG_INF("Enter identify mode");
        }
    } else {
        /* Cancel the toggling function alarm and turn off LED */
        err = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
        if (err) {
            LOG_ERR("Failed to schedule app alarm cancel: %d", err);
        } else {
            led_off(leds, ZIGBEE_NETWORK_STATE_LED);
            LOG_INF("Cancel identify mode");
        }
    }
}

/** @brief              Callback for button events.
 *  @param button_state Bitmask containing the state of the buttons
 *  @param has_changed  Bitmask containing buttons that have changed their state
 */
static void on_button_changed_handler(uint32_t button_state, uint32_t has_changed)
{
    LOG_DBG("Button pressed");

    if (IDENTIFY_MODE_BUTTON & has_changed) {
        if (IDENTIFY_MODE_BUTTON & button_state) {
            /* Button changed its state to pressed */
        } else {
            /* Button changed its state to released */
            if (was_factory_reset_done()) {
                /* The long press was for Factory Reset */
                LOG_DBG("After Factory Reset - ignore button release");
            } else {
                /* Button released before Factory Reset */
                LOG_DBG("Start identifying");

                /* Start identification mode */
                ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
            }
        }
    }

    check_factory_reset_button(button_state, has_changed);
}

/** @brief Function for initializing all clusters attributes.
 */
static void app_clusters_attr_init(void)
{
    /* Basic cluster attributes data */
    dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
    dev_ctx.basic_attr.app_version = INIT_BASIC_APP_VERSION;
    dev_ctx.basic_attr.stack_version = INIT_BASIC_STACK_VERSION;

#ifdef CONFIG_BOARD_REVISION
    dev_ctx.basic_attr.hw_version = (zb_uint8_t)atoi(CONFIG_BOARD_REVISION);
#else
    dev_ctx.basic_attr.hw_version = INIT_BASIC_HW_VERSION;
#endif /* CONFIG_BOARD_REVISION */

    /* Use ZB_ZCL_SET_STRING_VAL to set strings, because the first byte */
    /* should contain string length without trailing zero. */
    /* For example "test" string will be encoded as: */
    /*   [(0x4), 't', 'e', 's', 't'] */
    ZB_ZCL_SET_STRING_VAL(dev_ctx.basic_attr.mf_name, INIT_BASIC_MANUF_NAME,
                          ZB_ZCL_STRING_CONST_SIZE(INIT_BASIC_MANUF_NAME));

    ZB_ZCL_SET_STRING_VAL(dev_ctx.basic_attr.model_id, INIT_BASIC_MODEL_ID,
                          ZB_ZCL_STRING_CONST_SIZE(INIT_BASIC_MODEL_ID));

    ZB_ZCL_SET_STRING_VAL(dev_ctx.basic_attr.date_code, INIT_BASIC_DATE_CODE,
                          ZB_ZCL_STRING_CONST_SIZE(INIT_BASIC_DATE_CODE));

    dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;

    ZB_ZCL_SET_STRING_VAL(dev_ctx.basic_attr.location_id, INIT_BASIC_LOCATION_DESC,
                          ZB_ZCL_STRING_CONST_SIZE(INIT_BASIC_LOCATION_DESC));

    dev_ctx.basic_attr.ph_env = ZB_ZCL_BASIC_ENV_UNSPECIFIED;

    /* Identify cluster attributes data. */
    dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

    /* Temperature cluster attributes data. The value is unknown until the first measurement. */
    dev_ctx.temperature_attr.measurement_attr = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
    dev_ctx.temperature_attr.min_attr = THERMOCOUPLE_TEMPERATURE_MIN;
    dev_ctx.temperature_attr.max_attr = THERMOCOUPLE_TEMPERATURE_MAX;
    dev_ctx.temperature_attr.tolerance = 0;

    /* Initialize the values for the Battery cluster attributes */
    dev_ctx.power_attr.size = ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_BUILT_IN;
    dev_ctx.power_attr.quantity = 1;
    dev_ctx.power_attr.rated_voltage = THERMOCOUPLE_RATED_VOLTAGE_MV / 100;

    ZB_ZCL_SET_ATTRIBUTE(THERMOCOUPLE_ENDPOINT, ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                         ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
                         (zb_uint8_t *)&dev_ctx.power_attr.voltage, ZB_FALSE);

    ZB_ZCL_SET_ATTRIBUTE(THERMOCOUPLE_ENDPOINT, ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                         ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
                         (zb_uint8_t *)&dev_ctx.power_attr.percent_remaining, ZB_FALSE);
}

/** @brief  Run a single conversion of the MCP9600 in burst mode. The device enters the
 *          shutdown mode after the conversion, so it only draws current during a measurement.
 *          The caller must hold mcp9600_lock.
 *  @return 0 when successful
 */
static int mcp9600_burst(void)
{
    int ret;
    uint8_t status;

    /* The burst complete flag must be cleared before a new burst can be detected */
    ret = i2c_reg_update_byte_dt(&mcp9600_i2c, MCP9600_REG_STATUS, MCP9600_STATUS_BURST_COMPLETE,
                                 0);
    if (ret != 0) {
        return ret;
    }

    /* Start a burst with one sample (burst samples bits = 0) */
    ret = i2c_reg_update_byte_dt(&mcp9600_i2c, MCP9600_REG_DEV_CONFIG, MCP9600_MODE_MASK,
                                 MCP9600_MODE_BURST);
    if (ret != 0) {
        return ret;
    }

    for (int waited = 0; waited < MCP9600_BURST_TIMEOUT_MS; waited += MCP9600_BURST_POLL_MS) {
        k_msleep(MCP9600_BURST_POLL_MS);

        ret = i2c_reg_read_byte_dt(&mcp9600_i2c, MCP9600_REG_STATUS, &status);
        if (ret != 0) {
            return ret;
        }

        if (status & MCP9600_STATUS_BURST_COMPLETE) {
            return 0;
        }
    }

    return -ETIMEDOUT;
}

/** @brief              Read the thermocouple temperature from the MCP9600.
 *  @param milli_celsius Measured temperature in m°C
 *  @return             0 when successful
 */
static int temperature_read(int64_t *milli_celsius)
{
    int ret;
    struct sensor_value temperature;

    k_mutex_lock(&mcp9600_lock, K_FOREVER);

    ret = mcp9600_burst();
    if (ret != 0) {
        LOG_ERR("Can not start thermocouple conversion. Error %d", ret);
        goto out;
    }

    ret = sensor_sample_fetch(mcp9600);
    if (ret != 0) {
        LOG_ERR("Can not fetch thermocouple data. Error %d", ret);
        goto out;
    }

    ret = sensor_channel_get(mcp9600, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    if (ret != 0) {
        LOG_ERR("Can not read thermocouple temperature. Error %d", ret);
        goto out;
    }

    *milli_celsius = sensor_value_to_milli(&temperature);

out:
    k_mutex_unlock(&mcp9600_lock);

    return ret;
}

/** @brief  Convert a sensor value into 0.01 °C for printing without float support.
 *  @param  value Sensor value in °C
 *  @return Temperature in 0.01 °C
 */
static int32_t debug_centi_celsius(const struct sensor_value *value)
{
    return (int32_t)(sensor_value_to_milli(value) / 10);
}

/** @brief      Print all measurements of the MCP9600 periodically in debug builds. Runs in the
 *              system workqueue, so it works without a Zigbee connection.
 *              Hot = thermocouple tip, Cold = chip temperature, Delta = Hot - Cold,
 *              EMF = thermocouple voltage (2 uV per LSB at 18 bit resolution).
 *  @param work Unused parameter, required by the work queue API
 */
static void debug_print_handler(struct k_work *work)
{
    int ret;
    int32_t hot;
    int32_t cold;
    int32_t delta;
    uint8_t regs[3] = {0};
    struct sensor_value value[4];
    static const enum sensor_channel channels[] = {
        (enum sensor_channel)SENSOR_CHAN_MCP9600_HOT_JUNCTION_TEMP,
        (enum sensor_channel)SENSOR_CHAN_MCP9600_COLD_JUNCTION_TEMP,
        (enum sensor_channel)SENSOR_CHAN_MCP9600_DELTA_TEMP,
        (enum sensor_channel)SENSOR_CHAN_MCP9600_RAW_ADC,
    };

    ARG_UNUSED(work);

    k_mutex_lock(&mcp9600_lock, K_FOREVER);

    ret = mcp9600_burst();
    if (ret == 0) {
        ret = sensor_sample_fetch_chan(mcp9600, SENSOR_CHAN_ALL);
    }
    for (size_t i = 0; (ret == 0) && (i < ARRAY_SIZE(channels)); i++) {
        ret = sensor_channel_get(mcp9600, channels[i], &value[i]);
    }

    /* Status (0x04), thermocouple config (0x05) and device config (0x06) */
    for (size_t i = 0; (ret == 0) && (i < ARRAY_SIZE(regs)); i++) {
        ret = i2c_reg_read_byte_dt(&mcp9600_i2c, 0x04 + i, &regs[i]);
    }

    k_mutex_unlock(&mcp9600_lock);

    if (ret != 0) {
        LOG_ERR("Can not read MCP9600. Error %d", ret);
    } else {
        hot = debug_centi_celsius(&value[0]);
        cold = debug_centi_celsius(&value[1]);
        delta = debug_centi_celsius(&value[2]);

        LOG_INF("Hot: %s%d.%02d C, Cold: %s%d.%02d C, Delta: %s%d.%02d C, EMF: %d uV",
                (hot < 0) ? "-" : "", abs(hot) / 100, abs(hot) % 100, (cold < 0) ? "-" : "",
                abs(cold) / 100, abs(cold) % 100, (delta < 0) ? "-" : "", abs(delta) / 100,
                abs(delta) % 100, value[3].val1 * 2);
        LOG_INF("Status: 0x%02X, TC config: 0x%02X, Device config: 0x%02X", regs[0], regs[1],
                regs[2]);
    }

    k_work_schedule(&debug_print_work, K_MSEC(DEBUG_PRINT_INTERVAL_MS));
}

/** @brief Read the thermocouple temperature from the MCP9600 and update the Zigbee attribute.
 */
static void temperature_update(void)
{
    int64_t measured;
    int64_t milli_celsius;

    if (temperature_read(&milli_celsius) != 0) {
        return;
    }

    /* See chapter 4.4.2.2.1 of the ZCL Specification. The value is in 0.01 °C. */
    measured = CLAMP(milli_celsius / 10, THERMOCOUPLE_TEMPERATURE_MIN,
                     THERMOCOUPLE_TEMPERATURE_MAX);

    /* Only update when values differ */
    if (measured == dev_ctx.temperature_attr.measurement_attr) {
        return;
    }

    dev_ctx.temperature_attr.measurement_attr = (zb_int16_t)measured;

    ZB_ZCL_SET_ATTRIBUTE(THERMOCOUPLE_ENDPOINT, ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                         ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                         (zb_uint8_t *)&dev_ctx.temperature_attr.measurement_attr, ZB_FALSE);

    LOG_DBG("Temperature: %d", dev_ctx.temperature_attr.measurement_attr);
}

/** @brief Read the battery voltage and the charging state from the nPM1300 and update the
 *         Zigbee attributes.
 */
static void battery_update(void)
{
    int ret;
    bool is_charging;
    bool is_full;
    int32_t voltage_mv;
    int32_t remaining;
    struct sensor_value voltage;
    struct sensor_value status;

    ret = sensor_sample_fetch(charger);
    if (ret != 0) {
        LOG_ERR("Can not fetch charger data. Error %d", ret);
        return;
    }

    ret = sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &voltage);
    if (ret != 0) {
        LOG_ERR("Can not read battery voltage. Error %d", ret);
        return;
    }

    ret = sensor_channel_get(charger, (enum sensor_channel)SENSOR_CHAN_NPM13XX_CHARGER_STATUS,
                             &status);
    if (ret != 0) {
        LOG_ERR("Can not read charger status. Error %d", ret);
        return;
    }

    is_charging =
        (status.val1 & (CHARGER_STATUS_TRICKLE | CHARGER_STATUS_CC | CHARGER_STATUS_CV)) != 0;
    is_full = (status.val1 & CHARGER_STATUS_COMPLETE) != 0;
    voltage_mv = (int32_t)sensor_value_to_milli(&voltage);

    LOG_DBG("Battery: %d mV, charging: %s, full: %s", voltage_mv, is_charging ? "yes" : "no",
            is_full ? "yes" : "no");

    /* Only update when values differ */
    if ((voltage_mv / 100) == dev_ctx.power_attr.voltage) {
        return;
    }

    dev_ctx.power_attr.voltage = voltage_mv / 100;

    /* We need the difference between empty and full for the remaining percent */
    /* Formula: (Voltage - Min. Voltage) [V] / (Full voltage - Min. Voltage) [V] * 100 */
    remaining = voltage_mv - (int32_t)THERMOCOUPLE_EMPTY_VOLTAGE_MV;

    if (remaining < 0) {
        remaining = 0;
    } else {
        remaining = (remaining * 100) /
                    (int32_t)(THERMOCOUPLE_FULL_VOLTAGE_MV - THERMOCOUPLE_EMPTY_VOLTAGE_MV);
        if (remaining > 100) {
            remaining = 100;
        }
    }

    /* NOTE: A value of 200 is equal to 100%! */
    dev_ctx.power_attr.percent_remaining = remaining * 2;

    LOG_DBG("Battery voltage: %u", dev_ctx.power_attr.voltage);
    LOG_DBG("Battery remaining: %u", dev_ctx.power_attr.percent_remaining);

    ZB_ZCL_SET_ATTRIBUTE(THERMOCOUPLE_ENDPOINT, ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                         ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
                         (zb_uint8_t *)&dev_ctx.power_attr.voltage, ZB_FALSE);

    ZB_ZCL_SET_ATTRIBUTE(THERMOCOUPLE_ENDPOINT, ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                         ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
                         (zb_uint8_t *)&dev_ctx.power_attr.percent_remaining, ZB_FALSE);
}

/** @brief          Periodic measurement of all sensors. Runs in the context of the Zigbee stack,
 *                  so the attributes can be updated directly.
 *  @param bufid    Unused parameter, required by ZBOSS scheduler API
 */
static void measurement_handler(zb_bufid_t bufid)
{
    /* Start with a battery measurement after a reset */
    static uint32_t battery_elapsed_s = CONFIG_THERMOCOUPLE_BATTERY_INTERVAL_S;

    ZVUNUSED(bufid);

    /* The ZBOSS alarm can not handle long intervals (overflow after approx. 71 minutes), so the
     * battery interval is counted in steps of the measurement interval.
     */
    if (battery_elapsed_s >= CONFIG_THERMOCOUPLE_BATTERY_INTERVAL_S) {
        battery_elapsed_s = 0;
        battery_update();
    }

    battery_elapsed_s += CONFIG_THERMOCOUPLE_MEASUREMENT_INTERVAL_S;

    temperature_update();

    ZB_SCHEDULE_APP_ALARM(measurement_handler, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(MEASUREMENT_INTERVAL_MS));
}

/** @brief          Callback function for handling ZCL commands.
 *  @param bufid    Reference to Zigbee stack buffer used to pass received data
 */
static void on_zcl_cb_handler(zb_bufid_t bufid)
{
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

    if (device_cb_param->device_cb_id == ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID) {
#ifdef CONFIG_ZIGBEE_FOTA
        zigbee_fota_zcl_cb(bufid);
#endif /* CONFIG_ZIGBEE_FOTA */
    } else {
        device_cb_param->status = RET_NOT_IMPLEMENTED;
    }
}

/** @brief          Zigbee stack event handler.
 *  @param bufid    Reference to the Zigbee stack buffer used to pass signal
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
    zb_zdo_app_signal_hdr_t *p_sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &p_sg_p);
    zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

    /* Update network status LED. */
    switch (sig) {
    case ZB_SIGNAL_JOIN_DONE: {
        led_off(leds, ZIGBEE_NETWORK_STATE_LED);
        if (status == RET_OK) {
            for (uint8_t i = 0; i < 4; i++) {
                led_on(leds, ZIGBEE_NETWORK_STATE_LED);
                k_msleep(100);
                led_off(leds, ZIGBEE_NETWORK_STATE_LED);
                k_msleep(100);
            }
        }

        break;
    }
    case ZB_ZDO_SIGNAL_LEAVE: {
        for (uint8_t i = 0; i < 2; i++) {
            led_on(leds, ZIGBEE_NETWORK_STATE_LED);
            k_msleep(100);
            led_off(leds, ZIGBEE_NETWORK_STATE_LED);
            k_msleep(100);
        }

        break;
    }
    case ZB_ZDO_SIGNAL_SKIP_STARTUP: {
        LOG_DBG("Start the measurement of the sensors");
        ZB_SCHEDULE_APP_CALLBACK(measurement_handler, 0);

        break;
    }
    default: {
        break;
    }
    }

    /* No application-specific behavior is required. Call default signal handler. */
    ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

    /* All callbacks should either reuse or free passed buffers. */
    /* If bufid == 0, the buffer is invalid (not passed). */
    if (bufid) {
        zb_buf_free(bufid);
    }
}

int main(void)
{
    if (device_is_ready(mcp9600) == false) {
        LOG_ERR("MCP9600 device not ready");
        return -ENODEV;
    }

    if (device_is_ready(leds) == false) {
        LOG_ERR("nPM1300 LEDs not ready");
        return -ENODEV;
    }

    if (device_is_ready(charger) == false) {
        LOG_ERR("nPM1300 charger not ready");
        return -ENODEV;
    }

    /* The driver starts the MCP9600 in continuous mode. Keep it in shutdown between the
     * measurements, every measurement starts a single conversion in burst mode.
     */
    if (i2c_reg_update_byte_dt(&mcp9600_i2c, MCP9600_REG_DEV_CONFIG, MCP9600_MODE_MASK,
                               MCP9600_MODE_SHUTDOWN) != 0) {
        LOG_ERR("Can not put the MCP9600 into shutdown mode");
    }

    /* The nPM1300 keeps the state of the LED over a reset of the MCU */
    led_off(leds, ZIGBEE_NETWORK_STATE_LED);

    dk_buttons_init(on_button_changed_handler);

    register_factory_reset_button(FACTORY_RESET_BUTTON);

    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(1 * 60 * 1000));
    zb_zdo_pim_set_long_poll_interval(ZB_MILLISECONDS_TO_BEACON_INTERVAL(10 * 60 * 1000));

#ifdef CONFIG_ZIGBEE_FOTA
#error "Not supported yet!"
    /* Initialize Zigbee FOTA download service */
    zigbee_fota_init(on_zcl_ota_evt_handler);

    /* Mark the current firmware as valid */
    confirm_image();

    /* Register callback for handling ZCL commands */
    ZB_ZCL_REGISTER_DEVICE_CB(on_zcl_cb_handler);
#else
    ARG_UNUSED(on_zcl_cb_handler);
#endif /* CONFIG_ZIGBEE_FOTA */

    /* Register device context (endpoints) */
    ZB_AF_REGISTER_DEVICE_CTX(&thermocouple_ctx);

    app_clusters_attr_init();

    /* Register handler to identify notifications */
    ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(THERMOCOUPLE_ENDPOINT, on_Identify_Handler);

    /* Power off unused sections of RAM to lower device power consumption */
    zb_set_rx_on_when_idle(ZB_FALSE);
    if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
        power_down_unused_ram();
    }

    /* Print the temperature periodically, also when the device is not in a Zigbee network */
    if (IS_ENABLED(CONFIG_DEBUG)) {
        k_work_schedule(&debug_print_work, K_NO_WAIT);
    }

    /* Start Zigbee default thread */
    zigbee_enable();

    LOG_INF("Thermocouple application started");

    /* Put the MCU into power down mode */
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
