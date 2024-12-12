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




K_MUTEX_DEFINE  (mut1);
K_MUTEX_DEFINE  (mut2);


#define SLEEP_TIME_MS	1

//#define val = led_gpio

/* Configuração do botão e LED's */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led2)



#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;


#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif


#if !DT_NODE_HAS_STATUS_OKAY(LED1_NODE)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif





    uint32_t led;
    uint32_t cnt;





//struct para os leds
struct led {
    struct gpio_dt_spec spec;
    uint8_t num;
};

// configura para o led0
static const struct led led0 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
    .num = 0,
};

// configura para o led1
static const struct led led1 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
    .num = 1,
};




/*
 * The led1 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led_gpio = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios,
						     {0});

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	
}




void blink(const struct led *led, uint32_t sleep_ms, uint32_t id)
{
    const struct gpio_dt_spec *spec = &led->spec;
    int cnt = 0;
    int ret;


    if (!device_is_ready(spec->port)) {// Verifica se o dispositivo do LED está pronto
        printk("Error: %s device is not ready\n", spec->port->name);
        return;
    }


    ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT); // Configura o pino do LED
    if (ret != 0) {
        printk("Error %d: failed to configure pin %d (LED '%d')\n",
            ret, spec->pin, led->num);
        return;
    }


    while (1) {
        if(k_mutex_lock(&mut1, K_FOREVER) == 0){//tenta pegar o mutex
        gpio_pin_set(spec->port, spec->pin, cnt % 2);//troca o estado do led
        cnt++;//encrementa o contador


        k_msleep(sleep_ms);//espera


        gpio_pin_set(spec->port, spec->pin, cnt % 2);//troca o estado do led
		
        }
		//k_mutex_unlock(&mut1);
		//k_msleep(sleep_ms);
    }
}






void pisca(const struct led *led, uint32_t sleep_ms, uint32_t id){
    const struct gpio_dt_spec *spec = &led->spec;
    int cnt = 0;
    int ret;

    if (!device_is_ready(spec->port)) { // Verifica se o dispositivo do LED está pronto
        printk("Error: %s device is not ready\n", spec->port->name);
        return;
    }

    ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT); // Configura o pino do LED
    if (ret != 0) {
        printk("Error %d: failed to configure pin %d (LED '%d')\n", ret, spec->pin, led->num);
        return;
    }

    while (1) {
        int button_state = gpio_pin_get_dt(&button); // Obtém o estado do botão

        if (button_state > 0) { // Se o botão for pressionado
            if (k_mutex_lock(&mut1, K_NO_WAIT) != 0 && k_mutex_lock(&mut2, K_NO_WAIT) != 0) {
                // Se não conseguir pegar os mutexes, acende o LED
                gpio_pin_set(spec->port, spec->pin, 1); // Acende o LED
            } else {
                gpio_pin_set(spec->port, spec->pin, 0); // Apaga o LED
                // Se pegou os mutexes, desbloqueia-os
                k_mutex_unlock(&mut1);
                k_mutex_unlock(&mut2);
            }
        } else {
            gpio_pin_set(spec->port, spec->pin, 0); // Apaga o LED quando o botão não estiver pressionado
        }

        k_msleep(sleep_ms); // Aguarda um pouco
    }
}


int botao(void){
	int ret;

	if (!gpio_is_ready_dt(&button)) {//verifica se o button device está pronto
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);//configura o pino do botão como gpio input
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);//configura um interrupt para o pino do botão
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));//inicia o callback
	gpio_add_callback(button.port, &button_cb_data);//adiciona o callback
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	if (led_gpio.port && !gpio_is_ready_dt(&led_gpio)) {  // Usando led_gpio aqui
		printk("Error %d: LED device %s is not ready; ignoring it\n", ret, led_gpio.port->name);
		led_gpio.port = NULL;
	}
	if (led_gpio.port) {  // Usando led_gpio aqui
		ret = gpio_pin_configure_dt(&led_gpio, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n", ret, led_gpio.port->name, led_gpio.pin);
			led_gpio.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led_gpio.port->name, led_gpio.pin);
		}
	}

	printk("Press the button\n");
	if (led_gpio.port) {  // Usando led_gpio aqui
		while (1) {
			/* If we have an LED, match its state to the button's. */
			int val = gpio_pin_get_dt(&button);

			if (val >= 0) {
				if(k_mutex_lock(&mut2, K_FOREVER) == 0){
					gpio_pin_set_dt(&led_gpio, val);  // Usando led_gpio aqui
				}
					
			}
			else{
					k_mutex_unlock(&mut2);
					k_msleep(SLEEP_TIME_MS);
				}
			k_msleep(SLEEP_TIME_MS);
		}
	}
	return 0;
}









void blink0(void)
{
    blink(&led0, 100, 0);
}


void blink1(void)
{
    pisca(&led1, 1000, 1);
}

void butao(void)
{
    botao();
}





K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,
        PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
        PRIORITY, 0, 0);
K_THREAD_DEFINE(botao_id, STACKSIZE, butao, NULL, NULL, NULL,
        PRIORITY, 0, 0);
