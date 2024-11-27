

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led2)

K_MUTEX_DEFINE(my_mutex0);
K_MUTEX_DEFINE(my_mutex1);

struct printk_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	uint32_t led;
	uint32_t cnt;
};

K_FIFO_DEFINE(printk_fifo);

struct led {
	struct gpio_dt_spec spec;
	uint8_t num;
};

static const struct led led0 = {
	.spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
	.num = 0,
};

static const struct led led1 = {
	.spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
	.num = 1,
};
void verde(const struct led *led, uint32_t sleep_ms, uint32_t id)
{


	const struct gpio_dt_spec *spec = &led->spec;
	int cnt = 1;
	int ret;

	if (!device_is_ready(spec->port)) {
		printk("Error: %s device is not ready\n", spec->port->name);
		return;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d (LED '%d')\n",
			ret, spec->pin, led->num);
		return;
	}
	k_mutex_unlock(&my_mutex0);
	gpio_pin_set(spec->port, spec->pin, 0);
	while (1) {
		
		
		if (k_mutex_lock(&my_mutex0, K_MSEC(10)) == 0 ) {
    		/* mutex successfully locked */
			

			struct printk_data_t tx_data = { .led = id, .cnt = cnt };

			size_t size = sizeof(struct printk_data_t);
			char *mem_ptr = k_malloc(size);
			__ASSERT_NO_MSG(mem_ptr != 0);

			memcpy(mem_ptr, &tx_data, size);

			k_fifo_put(&printk_fifo, mem_ptr);
			gpio_pin_set(spec->port, spec->pin,1);
			k_msleep(sleep_ms);
			gpio_pin_set(spec->port, spec->pin, 0);
			printk("executing verde     \n");
			k_mutex_unlock(&my_mutex0);
			k_msleep(sleep_ms);
		} else {
    		printk("Cannot lock verde   \n");
			k_msleep(sleep_ms);
		}
	}
}


void vermelho(const struct led *led, uint32_t sleep_ms, uint32_t id)
{


	const struct gpio_dt_spec *spec = &led->spec;
	int cnt = 1;
	int ret;

	if (!device_is_ready(spec->port)) {
		printk("Error: %s device is not ready\n", spec->port->name);
		return;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure pin %d (LED '%d')\n",
			ret, spec->pin, led->num);
		return;
	}
	k_mutex_unlock(&my_mutex0);
	k_mutex_unlock(&my_mutex1);
	gpio_pin_set(spec->port, spec->pin, 0);
	while (1) {
		
		
		if (k_mutex_lock(&my_mutex0,K_FOREVER) == 0 && k_mutex_lock(&my_mutex1, K_FOREVER) == 0) {
    		/* mutex successfully locked */
			

			struct printk_data_t tx_data = { .led = id, .cnt = cnt };

			size_t size = sizeof(struct printk_data_t);
			char *mem_ptr = k_malloc(size);
			__ASSERT_NO_MSG(mem_ptr != 0);

			memcpy(mem_ptr, &tx_data, size);

			k_fifo_put(&printk_fifo, mem_ptr);
			gpio_pin_set(spec->port, spec->pin,1);
			k_msleep(sleep_ms);
			gpio_pin_set(spec->port, spec->pin, 0);
			printk("executing vermelho     \n");
			k_mutex_unlock(&my_mutex0);
			k_mutex_unlock(&my_mutex1);
			k_msleep(sleep_ms);
		} else {
    		printk("Cannot lock vermelho  \n");
			k_mutex_unlock(&my_mutex0);
			k_mutex_unlock(&my_mutex1);
			k_msleep(sleep_ms);
		}
	}
}
void blink0(void)
{
	verde(&led0, 500, 0);
}

void blink1(void)
{
	vermelho(&led1, 1000, 1);
}

void uart_out(void)
{
	while (1) {
		struct printk_data_t *rx_data = k_fifo_get(&printk_fifo,
							   K_FOREVER);
		printk("Toggled led%d; counter=%d\n",
		       rx_data->led, rx_data->cnt);
		k_free(rx_data);
	}
}

K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
		PRIORITY, 0, 0);

#define SLEEP_TIME_MS	1
/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios,
						     {0});

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}
void button_unpressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button unpressed at %" PRIu32 "\n", k_cycle_get_32());
}
int main(void)
{
	int ret;


	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);


	printk("Press the button\n");
	if (led.port) {
		while (1) {


			int val = gpio_pin_get_dt(&button);
			if (val == 0){
				gpio_pin_set_dt(&led, 0);
				k_mutex_unlock(&my_mutex1);
				printk("drop azul     \n");
			}
			if(val > 0){
				if (k_mutex_lock(&my_mutex1,K_FOREVER) == 0) {
						/* mutex successfully locked */
						/* If we have an LED, match its state to the button's. */
					
						gpio_pin_set_dt(&led, 1);
						printk("executing azul     \n");
				} else {
    			printf("Cannot lock azul\n");
				}
				
			}
			k_msleep(SLEEP_TIME_MS);
			
			
		}
		return 0;
	}
}	

