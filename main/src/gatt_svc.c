#include "gat_svc.h"
#include "common.h"
#include "led.h"
#include "motor.h"
#include "vl53l0x.h"

#define TAG "GATT SVC"

/* Private Variable */
static int send_data_value[5];

/* Private function declarations */
static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg);

static int joystickDataAccess_cb(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void* arg);

static int sensor_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void* arg);

/* Private variables */
/* Automation IO service */
static const ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static uint16_t led_chr_val_handle;
static const ble_uuid128_t led_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

/* Motor Conntrol Service */
static int motor_chr_value[2];
static const ble_uuid16_t motor_control_svc_uuid = BLE_UUID16_INIT(0x1812);
static uint16_t motor_chr_val_handle;
static const ble_uuid128_t motor_chr_uuid = BLE_UUID128_INIT(0x9a, 0x6f, 0x41, 0xd3, 0x8b, 0x4e, 0x4d, 0x2f, 
    0xa1, 0x29, 0xe9, 0x64, 0x71, 0xf5, 0xd0, 0x9c);


/* Sensor Service */
static const ble_uuid16_t sensor_svc_uuid = BLE_UUID16_INIT(0x1813);
static uint16_t sensor_chr_val_handle;
static const ble_uuid128_t sensor_chr_uuid = BLE_UUID128_INIT(0x9a, 0x6f, 0x41, 0xd3, 0x8b, 0x4e, 0x4d, 0x2f, 
    0xa1, 0x29, 0xe9, 0x64, 0x71, 0x05, 0x40, 0x3c);

/* Attrs for subscription */
static uint16_t sensor_chr_conn_handle = 0;
static bool sensor_chr_conn_handle_inited = false;
static bool sensor_ind_status = false;

/* GATT services */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Automation IO service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &auto_io_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* LED characteristic */
                .uuid = &led_chr_uuid.u,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .access_cb = led_chr_access,
                .val_handle = &led_chr_val_handle,
            },
            {
                0, /* No more characteristics in this service */
            },
        },
    },
    {
        /* Motor Control Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &motor_control_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* JoyStickData : Speed & Direction Control Instructions */
                .uuid = &motor_chr_uuid.u,
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE,
                .access_cb = joystickDataAccess_cb,
                .val_handle = &motor_chr_val_handle,
            },
            {
                0,
            },
        }
    },
    {
        /* Sensor Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &sensor_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* Sensor Data */
                .uuid = &sensor_chr_uuid.u,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
                .access_cb = sensor_chr_access_cb,
                .val_handle = &sensor_chr_val_handle,
            },
            {
                0,
            },
        },
    },
    {
        0, /* No more services */
    },
};

/* Private functions */
static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc = 0;

    /* Handle access events */
    /* Note: LED characteristic is write only */
    switch (ctxt->op) {

    /* Write characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG,
                     "characteristic write by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == led_chr_val_handle) {
            /* Verify access buffer length */
            if (ctxt->om->om_len == 1) {
                /* Turn the LED on or off according to the operation bit */
                if (ctxt->om->om_data[0]) {
                    // led_on();
                    ESP_LOGI(TAG, "Sensors Activated");
                    esp_timer_start_periodic(tof_timer, 100000);
                    // rf_toggle(1);
                } else {
                    // led_off();
                    ESP_LOGI(TAG, "Sensors Deactivated");
                    // rf_toggle(0);
                    esp_timer_stop(tof_timer);
                }
            } else {
                goto error;
            }
            return rc;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(TAG,
             "unexpected access operation to led characteristic, opcode: %d",
             ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int joystickDataAccess_cb(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void* arg){
    int rc = 0;

    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d",
                 conn_handle, attr_handle);
    } else {
        ESP_LOGI(TAG,
                 "characteristic write by nimble stack; attr_handle=%d",
                 attr_handle);
    }

    switch (ctxt->op) {

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* code */

        if (attr_handle == motor_chr_val_handle) {
            if (ctxt->om->om_len == 4) { // Expecting 4 bytes (2 for X, 2 for Y)
                int16_t x = (int16_t)(ctxt->om->om_data[0] | (ctxt->om->om_data[1] << 8));
                int16_t y = (int16_t)(ctxt->om->om_data[2] | (ctxt->om->om_data[3] << 8));
    
                motor_chr_value[0] = x;
                motor_chr_value[1] = -y;
    
                ESP_LOGI("JoyStick", "X: %d, Y: %d", motor_chr_value[0], motor_chr_value[1]);
    
                set_motor_control_value(motor_chr_value);
            } else {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
        } else {
            return BLE_ATT_ERR_UNLIKELY;
        }

        break;
    
    default:
        break;
    }

    return rc;
}

static int sensor_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void* arg) {
    /* Local variables */
    int rc = 0;

    /* Handle access events */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == sensor_chr_val_handle) {
            /* Update access buffer value */
            get_sensor_data(send_data_value);
            get_motor_control_value(send_data_value + 3);
            printf("Sensor data: %d %d %d %d %d\n", send_data_value[0],
                   send_data_value[1], send_data_value[2], send_data_value[3],
                   send_data_value[4]);
            rc = os_mbuf_append(ctxt->om, &send_data_value,
                                sizeof(send_data_value));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        return BLE_ATT_ERR_UNLIKELY;

    }
    return rc;
}

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == sensor_chr_val_handle) {
        /* Update heart rate subscription status */
        sensor_chr_conn_handle = event->subscribe.conn_handle;
        sensor_chr_conn_handle_inited = true;
        sensor_ind_status = event->subscribe.cur_indicate;
    }
}

/*Public functions*/

void send_sensor_value_indication(void) {
    if (sensor_ind_status && sensor_chr_conn_handle_inited) {
        ble_gatts_indicate(sensor_chr_conn_handle,
                           sensor_chr_val_handle);
        ESP_LOGI(TAG, "sensor value indication sent!");
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}