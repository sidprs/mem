/*
 * Custom kernel module for driving system health LED
 */
 
#include <linux/bits.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/module.h>
 
#define BIT(n) (1 << (n))
 
/*
 * Our GPIO expander supports four (4) I/O lines and has a very simple I2C
 * register interface with a single 8-bit register:
 *
 *        
 *        Bit range        Register Name      Description
 *        ---------        -------------      -----------
 *        7:4              DIRECTION          Pin direction
 *                                            for I/O pins 3:0
 *                                            (0: Input, 1: Output)
 *
 *        3:0              VALUE              Pin value for I/O pins 3:0
 *                                            Inputs: Indicates line state
 *                                            Outputs: Set line state
 *                                            (0: Low, 1: High)
 */
 
// 0 1 2 3 4 5 6 7
// V V V V D D D D
// 0 1 2 3 0 1 2 3
// 0 0 0 0 0 1 0 0  D: output

// X 1 X X
// X 0 X X 

// X X X X X 0 X X
/* Bits 7:4 are used for pin directions */
#define REG_MASK_DIRECTION        0xf0
 
/* Bits 3:0 are used for pin value */
#define REG_MASK_VALUE            0x0f
 
/* The GPIO I/O pin number wired up to our health LED */
#define HEALTH_DEV_LED_PIN        2
 
/* CPU's interrupt line tied to our FAULT# signal */
#define SOC_FAULT_IRQ_NUM        42
 
 
/**
 * struct health_dev - State container for our I2C GPIO expander
 * @chip: GPIO chip instance
 * @client: I2C client
 * @shadow: shadow register to store previously known state
 */
struct health_dev {
        struct gpio_chip chip;
        struct i2c_client *client;
        u8 shadow; // copy of register
};
 
static struct health_dev *sys_health_dev = NULL;
 
/*
 * Configure GPIO line as input
 *   @offset: GPIO pin number (0-3) to set as input
 */
static int gpio_expander_set_dir_input(struct gpio_chip *gc, unsigned offset)
{
        struct health_dev *dev = gpiochip_get_data(gc);
        // direction 7:4
        
        /* Modify appropriate bits in shadow register to set direction */
        dev->shadow &= ~(1U << ( offset + 4)) 

        /* I2C write */
        return i2c_smbus_write_byte(dev->client, dev->shadow);
}
 
/*
 * Read current GPIO line states
 *   @offset: GPIO pin number (0-3) to read state of
 *
 * Returns 1 for HIGH, 0 for LOW
 */
static int gpio_expander_get(struct gpio_chip *gc, unsigned offset)
{
        struct health_dev *dev = gpiochip_get_data(gc);
        s32 val;
 
        val = i2c_smbus_read_byte(dev->client);
        
        return (val >> offset) & 0x1;
 
        /* XXX: TODO: Return 1 if line is HIGH, 0 if LOW */
}
 
/*
 * Configure GPIO line as an output:
 *    @offset: GPIO pin number (0-3) to set as output
 *    @value: Initial output state (1: HIGH, 0: LOW)
 */
static int gpio_expander_set_dir_output(struct gpio_chip *gc, unsigned offset, int value)
{
        struct health_dev *dev = gpiochip_get_data(gc);
 
 
        /* XXX: TODO: Update dev->shadow appropriately */
 
 
        /* I2C write */
        return i2c_smbus_write_byte(dev->client, dev->shadow);
}
 
/* Set GPIO output line states */
static void gpio_expander_set(struct gpio_chip *gc, unsigned offset, int value)
{
        gpio_expander_set_dir_output(gc, offset, value);
}
 
static int health_dev_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
        struct device *i2c_dev = &client->dev;
        struct device_node *np = dev->of_node;
        struct health_dev *dev;
        int ret;
 
        dev = devm_kzalloc(i2c_dev, sizeof(*dev), GFP_KERNEL);
        if (!dev)
                return -ENOMEM;
 
        dev->chip.base = -1;
        dev->chip.can_sleep = true;
        dev->chip.parent = i2c_dev;
        dev->chip.of_node = np;
        dev->chip.owner = THIS_MODULE;
        dev->chip.label = dev_name(i2c_dev);
        dev->chip.ngpio = 8;
        dev->chip.direction_input = gpio_expander_set_dir_input;
        dev->chip.get = gpio_expander_get;
        dev->chip.direction_output = gpio_expander_set_dir_output;
        dev->chip.set = gpio_expander_set;
        dev->client = client;
 
        client->flags |= I2C_M_IGNORE_NAK;
        dev->shadow = 0xff;
 
        i2c_set_clientdata(client, dev);
 
        ret = devm_gpiochip_add_data(i2c_dev, &dev->chip, dev);
        if (ret)
                return ret;
 
        dev_info(i2c_dev, "registered health module\n");
 
        return 0;
}
 
/*
 * Handle SoC FAULT# IRQ
 */
static irqreturn_t fault_irq_handler(int irq, void *data)
{
        /* XXX: */
 
        return IRQ_HANDLED;
}
 
static const struct i2c_device_id health_dev_id[] = {
        { "health-dev", },
        { }
};
MODULE_DEVICE_TABLE(i2c, health_dev_id);
 
static const struct of_device_id health_dev_dt_ids[] = {
        { .compatible = "tesla,health-dev", },
        { },
};
MODULE_DEVICE_TABLE(of, health_dev_dt_ids);
 
static struct i2c_driver health_dev_driver = {
        .driver = {
                .name = "health_dev",
                .of_match_table = health_dev_dt_ids,
        },
        .probe = health_dev_probe,
        .id_table = health_dev_id,
};
 
static int __init health_dev_init(void)
{
        int res;
 
        res = i2c_add_driver(&health_dev_driver);
        if (res != 0)
                return res;
 
        res = request_irq(SOC_FAULT_IRQ_NUM, fault_irq_handler,
                          IRQF_SHARED, "fault", NULL);
 
        return 0;
}
 
static void __exit health_dev_exit(void)
{
        i2c_del_driver(&health_dev_driver);
}
 
subsys_initcall(health_dev_init);
module_exit(health_dev_exit); 
