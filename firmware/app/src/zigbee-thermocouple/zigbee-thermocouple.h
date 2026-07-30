/*
 * This file is part of the Zigbee-Thermocouple project <https://github.com/Kampi/zigbee-thermocouple>.
 * Copyright (c) 2026 Daniel Kampert.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

/* Version of the application software (1 byte). */
#define INIT_BASIC_APP_VERSION                  1

/* Version of the implementation of the Zigbee stack (1 byte). */
#define INIT_BASIC_STACK_VERSION                10

/* Version of the hardware of the device (1 byte). */
#define INIT_BASIC_HW_VERSION                   1

/* Manufacturer name (32 bytes). */
#define INIT_BASIC_MANUF_NAME                   "Kampi"

/* Model number assigned by manufacturer (32-bytes long string). */
#define INIT_BASIC_MODEL_ID                     "Zigbee-Thermocouple_v1"

/* First 8 bytes specify the date of manufacturer of the device
 * in ISO 8601 format (YYYYMMDD). The rest (8 bytes) are manufacturer specific.
 */
#ifdef CONFIG_DEBUG
#define INIT_BASIC_DATE_CODE                    "20251019_Debug"
#else
#define INIT_BASIC_DATE_CODE                    "20251019_Release"
#endif

/* Describes the physical location of the device (16 bytes).
 * May be modified during commissioning process.
 */
#define INIT_BASIC_LOCATION_DESC                "Office desk"

/* Button used to enter the Identify mode. */
#define IDENTIFY_MODE_BUTTON                    DK_BTN1_MSK

/* LED indicating that light switch successfully joined Zigbee network. */
#define ZIGBEE_NETWORK_STATE_LED                DK_LED1

/* Button to start Factory Reset. */
#define FACTORY_RESET_BUTTON                    IDENTIFY_MODE_BUTTON

/* Define 'bat_num' as empty in order to declare default battery set attributes. */
/* According to Table 3-17 of ZCL specification, defining 'bat_num' as 2 or 3 allows */
/* to declare battery set attributes for BATTERY2 and BATTERY3. */
#define bat_num

/** @brief Device endpoint, used to receive environment sensor results.
 */
#define ZIGBEE_THERMOCOUPLE_ENDPOINT            10

/** @brief The rated battery voltage in millivolts.
 */
#define ZIGBEE_THERMOCOUPLE_RATED_VOLTAGE_MV    3300UL

/** @brief The lowest battery voltage in millivolts.
 */
#define ZIGBEE_THERMOCOUPLE_EMPTY_VOLTAGE_MV    2700UL

/** @brief ZigBee device version.
*/
#define ZB_DEVICE_VER                           1

/** @brief  ZigBee profile ID used
 *          See ZIGBEE HOME AUTOMATION PUBLIC APPLICATION PROFILE table 5.1 for additional information.
*/
#define ZB_PROFILE_ID                           ZB_AF_HA_PROFILE_ID

/** @brief  ZigBee device ID
 *          See ZIGBEE HOME AUTOMATION PUBLIC APPLICATION PROFILE table 5.1 for additional information.
*/
#define ZB_DEVICE_ID                            0x106

/** @brief See https://zigbeealliance.org/wp-content/uploads/2021/10/07-5123-08-Zigbee-Cluster-Library.pdf for additional informations
 *  @details
 *          Server
 *              - @ref ZB_ZCL_BASIC \n
 *              - @ref ZB_ZCL_IDENTIFY \n
 *              - @ref ZB_ZCL_CLUSTER_ID_POWER_CONFIG \n
 *              - @ref ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT \n
 *
 *          Client
 *              -
 */

/** @brief Number of IN (server) clusters.
*/
#define ZIGBEE_THERMOCOUPLE_IN_CLUSTER_NUM      7

/** @brief Number of OUT (client) clusters.
*/
#define ZIGBEE_THERMOCOUPLE_OUT_CLUSTER_NUM     1

/** @brief Number of attributes.
*/
#define ZIGBEE_THERMOCOUPLE_REPORT_ATTR_COUNT   7

/** @brief                  Declare endpoint for the Environment Sensor device.
 *  @param ep_name          Endpoint variable name
 *  @param ep_id            Endpoint ID
 *  @param cluster_list     Endpoint cluster list
 */
#define ZIGBEE_THERMOCOUPLE_DECLARE_EP(ep_name, ep_id, cluster_list)            \
    ZIGBEE_THERMOCOUPLE_DECLARE_SIMPLE_DESC(                                    \
        ep_name,                                                                \
        ep_id,                                                                  \
        ZIGBEE_THERMOCOUPLE_IN_CLUSTER_NUM,                                     \
        ZIGBEE_THERMOCOUPLE_OUT_CLUSTER_NUM                                     \
    );                                                                          \
    ZBOSS_DEVICE_DECLARE_REPORTING_CTX(                                         \
        reporting_info##ep_name,                                                \
        ZIGBEE_THERMOCOUPLE_REPORT_ATTR_COUNT                                   \
    );                                                                          \
    ZB_AF_DECLARE_ENDPOINT_DESC(                                                \
        ep_name,                                                                \
        ep_id,                                                                  \
        ZB_PROFILE_ID,                                                          \
        0,                                                                      \
        NULL,                                                                   \
        ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t),                 \
        cluster_list,                                                           \
        (zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,                      \
        ZIGBEE_THERMOCOUPLE_REPORT_ATTR_COUNT,                                  \
        reporting_info##ep_name,                                                \
        0,                                                                      \
        NULL                                                                    \
    )

/** @brief 3.3 Power Attributes
*/
typedef struct {
    zb_uint8_t voltage;                 /**< Attribute 3.3.2.2.3.1 */
    zb_uint8_t size;                    /**< Attribute 3.3.2.2.4.2 */
    zb_uint8_t quantity;                /**< Attribute 3.3.2.2.4.4 */
    zb_uint8_t rated_voltage;           /**< Attribute 3.3.2.2.4.5 */
    zb_uint8_t alarm_mask;              /**< Attribute 3.3.2.2.4.6 */
    zb_uint8_t voltage_min_threshold;   /**< Attribute 3.3.2.2.4.7 */
    zb_uint8_t percent_remaining;       /**< Attribute 3.3.2.2.3.1
                                             NOTE: A value of 200  is equal to 100%! */
    zb_uint8_t voltage_threshold_1;     /**< Attribute 3.3.2.2.4.8 */
    zb_uint8_t voltage_threshold_2;     /**< Attribute 3.3.2.2.4.8 */
    zb_uint8_t voltage_threshold_3;     /**< Attribute 3.3.2.2.4.8 */
    zb_uint8_t percent_min_threshold;   /**< Attribute 3.3.2.2.4.9 */
    zb_uint8_t percent_threshold_1;     /**< Attribute 3.3.2.2.4.10 */
    zb_uint8_t percent_threshold_2;     /**< Attribute 3.3.2.2.4.10 */
    zb_uint8_t percent_threshold_3;     /**< Attribute 3.3.2.2.4.10 */
    zb_uint32_t alarm_state;            /**< Attribute 3.3.2.2.4.11 */
} zb_zcl_power_attrs_t;

/** @brief 4.4 Temperature Measurement
*/
typedef struct {
    zb_uint16_t measurement_attr;       /**< Attribute 4.4.2.2.1 */
    zb_uint16_t min_attr;               /**< Attribute 4.4.2.2.2 */
    zb_uint16_t max_attr;               /**< Attribute 4.4.2.2.3 */
    zb_uint16_t tolerance;              /**< Attribute 4.4.2.2.4 */
} zb_zcl_temperature_attrs_t;

/** @brief Main application customizable context. Stores all settings and static values.
 */
typedef struct {
    zb_zcl_basic_attrs_ext_t basic_attr;
    zb_zcl_identify_attrs_t identify_attr;
    zb_zcl_groups_attrs_t groups_attr;
    zb_zcl_power_attrs_t power_attr;
    zb_zcl_temperature_attrs_t temperature_attr;
} device_ctx_t;

#define ZIGBEE_THERMOCOUPLE_CLUSTER_BASIC_DESC(basic_attr_list) \
    ZB_ZCL_CLUSTER_DESC( \
        ZB_ZCL_CLUSTER_ID_BASIC, \
        ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t), \
        (basic_attr_list), \
        ZB_ZCL_CLUSTER_SERVER_ROLE, \
        ZB_ZCL_MANUF_CODE_INVALID \
    ),

#define ZIGBEE_THERMOCOUPLE_CLUSTER_IDENTIFY_SERVER_DESC(identify_server_attr_list) \
    ZB_ZCL_CLUSTER_DESC( \
        ZB_ZCL_CLUSTER_ID_IDENTIFY, \
        ZB_ZCL_ARRAY_SIZE(identify_server_attr_list, zb_zcl_attr_t), \
        (identify_server_attr_list), \
        ZB_ZCL_CLUSTER_SERVER_ROLE, \
        ZB_ZCL_MANUF_CODE_INVALID \
    ),

#define ZIGBEE_THERMOCOUPLE_CLUSTER_POWER_DESC(power_attr_list) \
    ZB_ZCL_CLUSTER_DESC( \
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG, \
        ZB_ZCL_ARRAY_SIZE(power_attr_list, zb_zcl_attr_t), \
        (power_attr_list), \
        ZB_ZCL_CLUSTER_SERVER_ROLE, \
        ZB_ZCL_MANUF_CODE_INVALID \
    ),

#define ZIGBEE_THERMOCOUPLE_CLUSTER_TEMP_DESC(temp_attr_list) \
    ZB_ZCL_CLUSTER_DESC( \
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, \
        ZB_ZCL_ARRAY_SIZE(temp_attr_list, zb_zcl_attr_t), \
        (temp_attr_list), \
        ZB_ZCL_CLUSTER_SERVER_ROLE, \
        ZB_ZCL_MANUF_CODE_INVALID \
    ),

#define ZIGBEE_THERMOCOUPLE_CLUSTER_IDENTIFY_CLIENT_DESC(identify_client_attr_list) \
    ZB_ZCL_CLUSTER_DESC( \
        ZB_ZCL_CLUSTER_ID_IDENTIFY, \
        ZB_ZCL_ARRAY_SIZE(identify_client_attr_list, zb_zcl_attr_t),    \
        (identify_client_attr_list),    \
        ZB_ZCL_CLUSTER_CLIENT_ROLE, \
        ZB_ZCL_MANUF_CODE_INVALID   \
    ),  \

/** @brief                              Declare the cluster list for the sensor device.
 *  @param cluster_list_name            Cluster list variable name
 *  @param basic_attr_list              Attribute list for Basic cluster
 *  @param identify_client_attr_list    Attribute list for Identify cluster (Client)
 *  @param identify_server_attr_list    Attribute list for Identify cluster (Server)
 *  @param power_attr_list              Attribute list for Power cluster
 *  @param temp_attr_list               Attribute list for Temperature Measurement cluster
 */
#define ZIGBEE_THERMOCOUPLE_DECLARE_CLUSTER_LIST(                           \
    cluster_list_name,                                                      \
    basic_attr_list,                                                        \
    identify_client_attr_list,                                              \
    identify_server_attr_list,                                              \
    power_attr_list,                                                        \
    temp_attr_list,                                                         \
    static zb_zcl_cluster_desc_t cluster_list_name[] =                      \
    {                                                                       \
        ZIGBEE_THERMOCOUPLE_CLUSTER_BASIC_DESC(basic_attr_list)                        \
        ZIGBEE_THERMOCOUPLE_CLUSTER_IDENTIFY_SERVER_DESC(identify_server_attr_list)    \
        ZIGBEE_THERMOCOUPLE_CLUSTER_POWER_DESC(power_attr_list)                        \
        ZIGBEE_THERMOCOUPLE_CLUSTER_TEMP_DESC(temp_attr_list)                          \
        ZIGBEE_THERMOCOUPLE_CLUSTER_IDENTIFY_CLIENT_DESC(identify_client_attr_list)    \
    }

/** @brief                  Declare simple descriptor for the sensor device.
 *  @param ep_name          Endpoint variable name
 *  @param ep_id            Endpoint ID
 *  @param in_clust_num     Number of supported input clusters
 *  @param out_clust_num    Number of supported output clusters
 */
#define ZIGBEE_THERMOCOUPLE_DECLARE_SIMPLE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
    ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num); \
    ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name = \
    { \
        ep_id, \
        ZB_PROFILE_ID, \
        ZB_DEVICE_ID, \
        ZB_DEVICE_VER, \
        0, \
        in_clust_num, \
        out_clust_num, \
        { \
            ZB_ZCL_CLUSTER_ID_BASIC, \
            ZB_ZCL_CLUSTER_ID_IDENTIFY, \
            ZB_ZCL_CLUSTER_ID_POWER_CONFIG, \
            ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, \
            ZB_ZCL_CLUSTER_ID_IDENTIFY \
        } \
    }
