#ifndef __CUST_SENSORHUB_H__
#define __CUST_SENSORHUB_H__

#define G_CUST_I2C_ADDR_NUM 2

struct sensorHub_hw {
	int is_batch_enabled;
};

static struct sensorHub_hw cust_sensorHub_hw = {
    .is_batch_enabled = true,
};
struct sensorHub_hw *get_cust_sensorHub_hw(void) {
    return &cust_sensorHub_hw;
}

//extern struct sensorHub_hw *get_cust_sensorHub_hw(void);
#endif
