#include <stdio.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <sensor_msgs/msg/imu.h>
#include <geometry_msgs/msg/vector3.h>
#include <rmw_microros/rmw_microros.h>

#include "pico/stdlib.h"
#include "pico_uart_transports.h"

#define HISTORY_SIZE 5

const uint LED_PIN = 25;

struct IMU_flatten
{
    float linear_acceleration_x;
    float linear_acceleration_y;
    float linear_acceleration_z;
};

rcl_publisher_t publisher;
rcl_publisher_t publisher_filtered;
rcl_subscription_t subscriber;

sensor_msgs__msg__Imu msg_in_type;
geometry_msgs__msg__Vector3 msg_out;
geometry_msgs__msg__Vector3 msg_out_filtered;

struct IMU_flatten historyNumbers[HISTORY_SIZE] = {0};
int historyCounter = 0;

void IMU_flatten_add(struct IMU_flatten *sum, struct IMU_flatten *add)
{
    sum->linear_acceleration_x += add->linear_acceleration_x;
    sum->linear_acceleration_y += add->linear_acceleration_y;
    sum->linear_acceleration_z += add->linear_acceleration_z;
}

void IMU_Flatten_divide_by_value(struct IMU_flatten *sum, float divider)
{
    sum->linear_acceleration_x /= divider;
    sum->linear_acceleration_y /= divider;
    sum->linear_acceleration_z /= divider;
}

void msg_callback(const void *msgin)
{
    const sensor_msgs__msg__Imu *msg = (const sensor_msgs__msg__Imu *)msgin;

    if (historyCounter < HISTORY_SIZE)
    {

        struct IMU_flatten s = {
            msg->linear_acceleration.x,
            msg->linear_acceleration.y,
            msg->linear_acceleration.z};

        historyNumbers[historyCounter] = s;
        historyCounter++;
        return;
    }

    struct IMU_flatten sum = {0.0, 0.0, 0.0};

    for (int pos = 0; pos < HISTORY_SIZE - 1; pos++)
    {
        historyNumbers[pos] = historyNumbers[pos + 1];
        IMU_flatten_add(&sum, &historyNumbers[pos]);
    }

    struct IMU_flatten s = {
        msg->linear_acceleration.x,
        msg->linear_acceleration.y,
        msg->linear_acceleration.z};

    historyNumbers[HISTORY_SIZE - 1] = s;

    IMU_flatten_add(&sum, &historyNumbers[HISTORY_SIZE - 1]);
    IMU_Flatten_divide_by_value(&sum, (float)HISTORY_SIZE);

    msg_out.x = msg->linear_acceleration.x;
    msg_out.y = msg->linear_acceleration.y;
    msg_out.z = msg->linear_acceleration.z;
    
    msg_out_filtered.x = sum.linear_acceleration_x;
    msg_out_filtered.y = sum.linear_acceleration_y;
    msg_out_filtered.z = sum.linear_acceleration_z;

    rcl_publish(&publisher, (const void *)&msg_out, NULL);
    rcl_publish(&publisher_filtered, (const void *)&msg_out_filtered, NULL);
}

int main()
{
    rmw_uros_set_custom_transport(
        true,
        NULL,
        pico_serial_transport_open,
        pico_serial_transport_close,
        pico_serial_transport_write,
        pico_serial_transport_read);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    rcl_timer_t timer;
    rcl_node_t node;
    rcl_allocator_t allocator;
    rclc_support_t support;
    rclc_executor_t executor;

    allocator = rcl_get_default_allocator();

    // Wait for agent successful ping for 2 minutes.
    const int timeout_ms = 1000;
    const uint8_t attempts = 120;

    rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);

    if (ret != RCL_RET_OK)
    {
        // Unreachable agent, exiting program.
        return ret;
    }

    rclc_support_init(&support, 0, NULL, &allocator);

    rclc_node_init_default(&node, "pico_node", "", &support);

    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Vector3),
        "/microROS/acc");

    rclc_publisher_init_default(
        &publisher_filtered,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Vector3),
        "/microROS/acc_sma");

    rclc_subscription_init_best_effort(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
        "/lunarsim/imu");

    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(
        &executor,
        &subscriber,
        &msg_in_type,
        &msg_callback,
        ON_NEW_DATA);

    gpio_put(LED_PIN, 1);

    rclc_executor_spin(&executor);

    return 0;
}
