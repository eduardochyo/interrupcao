
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

//associa o pino led1 e led 2 às variáveis LED1_NODE e LED0_NODE
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led2)

//Define e inicia os mutexes
K_MUTEX_DEFINE(my_mutex0);
K_MUTEX_DEFINE(my_mutex1);

//define a classe led e define suas propriedades
struct led {
	struct gpio_dt_spec spec; // inicia classe de especificações espec
	uint8_t num;
};

static const struct led led0 = { //cria o elemento led 0 da classe led
	.spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}), //obtem dados da devicetree sobre associada a LED0_NODE e define o pino como gpio
	.num = 0, //se o LED0_NODE não estiver definido spec = 0
};
static const struct led led1 = {
	.spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
	.num = 1,
};
//inicia a função verde com parametros o elemento led, a variavel sleep_ms a variavel id
void verde(const struct led *led, uint32_t sleep_ms, uint32_t id)
{
	//cria e define o ponteiro espec e associa as informações contidas no elemento espec no elemento led
	const struct gpio_dt_spec *spec = &led->spec;

	int ret;
	//configura o pino do associado com spec como pino de saída
	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);

	//seta o valor do pino como 0
	gpio_pin_set(spec->port, spec->pin, 0);

	while (1) {
		//verifica se o mutex etá diponível, se estiver pega e entra no if
		if (k_mutex_lock(&my_mutex0, K_MSEC(10)) == 0 ) {
    		/* mutex successfully locked */
			//seta o pino como 1
			gpio_pin_set(spec->port, spec->pin,1);

			//coloca a thread em waiting por tempo de sleep_ms
			k_msleep(sleep_ms);
			
			gpio_pin_set(spec->port, spec->pin, 0);

			//printa no terminal a frase 
			printk("executing verde     \n");

			//libera o mutex
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
	int ret;

	ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);

	k_mutex_unlock(&my_mutex0);
	k_mutex_unlock(&my_mutex1);
	gpio_pin_set(spec->port, spec->pin, 0);
	while (1) {
		
		
		if (k_mutex_lock(&my_mutex0,K_MSEC(100)) == 0 && k_mutex_lock(&my_mutex1, K_MSEC(100)) == 0) {
    		/* mutex successfully locked */

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
//é a função da thread
void blink0(void) 
{//chama a função verde e define o elemento led, o sleep_ms e define o id
	verde(&led0, 500, 0);
}

void blink1(void)
{
	vermelho(&led1, 1000, 1);
}
//inicializa e cria uma thread, define o nome, o tamnho, a função que será executada e a prioridade
K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
		PRIORITY, 0, 0);

//Código do botão
//associa o pino sw0 2 à variável SW0_NODE
#define SW0_NODE	DT_ALIAS(sw0)

//obtem dados da devicetree sobre associada a SW0_NODE e define o pino como gpio
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0}); //se o LED0_NODE não estiver definido spec = 0

//define um elemento que armazena button_cb_data						  
static struct gpio_callback button_cb_data;

//cria e define o elemento led e defini como gpio
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios,
						     {0});

//o ponteiro dev é o spec do buttom, gpio callback armazena os valores de cb 
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

void interrupcao (void){
	//declara a variável que guarda o retorno
	int ret;

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

	gpio_add_callback(button.port, &button_cb_data);
}

K_THREAD_DEFINE(button, STACKSIZE, interrupcao , NULL, NULL, NULL,
		PRIORITY, 0, 0);