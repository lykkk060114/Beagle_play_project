#ifndef SENSOR_NODE_TYPES_H
#define SENSOR_NODE_TYPES_H

#include <zephyr/drivers/sensor.h>

struct sensor_data {
    struct sensor_value temperature;
    struct sensor_value humidity;
    struct sensor_value light;
};

#endif // SENSOR_NODE_TYPES_H
