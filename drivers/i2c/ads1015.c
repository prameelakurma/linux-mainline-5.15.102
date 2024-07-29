#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-smbus.h>
#include <linux/of.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define ADC_REG 0x49

static int adc_read(struct i2c_client *client)
{
    int ret;
    u8 buf[2];

    ret = i2c_smbus_read_i2c_block_data(client, ADC_REG, sizeof(buf), buf);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read ADC data: %d\n", ret);
        return ret;
    }

    return (buf[0] << 8) | buf[1];
}

static int adc_read_raw(struct iio_dev *indio_dev,
                        struct iio_chan_spec const *chan,
                        int *val, int *val2, long mask)
{
    struct i2c_client *client = iio_priv(indio_dev);
    int ret;

    switch (mask) {
        case IIO_CHAN_INFO_RAW:
            ret = adc_read(client);
            if (ret < 0)
                return ret;
            *val = ret;
            return IIO_VAL_INT;

        default:
            return -EINVAL;
    }
}

static const struct iio_info adc_info = {
    .read_raw = adc_read_raw,
};

static int adc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct iio_dev *indio_dev;
    struct iio_chan_spec *channels;
    int ret;

    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*client));
    if (!indio_dev)
        return -ENOMEM;

    indio_dev->name = "ads1015";
    indio_dev->info = &adc_info;
    indio_dev->modes = INDIO_DIRECT_MODE;
    
    channels = devm_kcalloc(&client->dev, 2, sizeof(struct iio_chan_spec), GFP_KERNEL);
    if (!channels)
        return -ENOMEM;

    channels[0].type = IIO_VOLTAGE;
    channels[0].indexed = 1;
    channels[0].channel = 0;
    channels[0].address = ADC_REG;
    channels[0].info_mask_separate = BIT(IIO_CHAN_INFO_RAW);
    
    channels[1].type = IIO_VOLTAGE;
    channels[1].indexed = 1;
    channels[1].channel = 1;
    channels[1].address = ADC_REG + 1; // Assuming channel 1 is at next register, adjust as needed
    channels[1].info_mask_separate = BIT(IIO_CHAN_INFO_RAW);

    indio_dev->channels = channels;
    indio_dev->num_channels = 2;

    i2c_set_clientdata(client, indio_dev);

    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        dev_err(&client->dev, "Failed to register IIO device: %d\n", ret);
        return ret;
    }

    dev_info(&client->dev, "ADC device registered\n");

    return 0;
}

static int adc_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);

    iio_device_unregister(indio_dev);

    return 0;
}

static const struct i2c_device_id adc_id[] = {
    { "ads1015", 1 },
    { }
};
MODULE_DEVICE_TABLE(i2c, adc_id);

static const struct of_device_id adc_of_match[] = {
    { .compatible = "ti,ads1015" },
    { }
};
MODULE_DEVICE_TABLE(of, adc_of_match);

static struct i2c_driver adc_driver = {
    .driver = {
        .name = "ads1015",
        .of_match_table = adc_of_match,
    },
    .probe = adc_probe,
    .remove = adc_remove,
    .id_table = adc_id,
};

module_i2c_driver(adc_driver);

MODULE_DESCRIPTION("I2C ADC Driver");
MODULE_LICENSE("GPL");

