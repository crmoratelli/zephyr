#include <zephyr.h>

#include <misc/__assert.h>

#define STACKSIZE 1024

K_THREAD_STACK_DEFINE(stack_prod, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_cons, STACKSIZE);

#define SIZE 8
unsigned int buffer[SIZE];
unsigned int in = 0;
unsigned int out = 0;

struct k_sem prod;
struct k_sem cons;
struct k_sem mutex;

struct k_thread __kernel tprod;
struct k_thread __kernel tcons;

static int get_random_int(int id, int period_in_ms){
	/*
	 * The random delay is unit-less, and is based on the philosopher's ID
	 * and the current uptime to create some pseudo-randomness. It produces
	 * a value between 0 and 31.
	 */
	k_enable_sys_clock_always_on();
	int delay = (k_uptime_get_32()/100 * (id + 1)) & 0x1f;
	k_disable_sys_clock_always_on();

	/* add 1 to not generate a delay of 0 */
	int ms = (delay + 1) * period_in_ms;

	return ms;
}


void set_to_buffer(int v){
	
	k_sem_take(&prod, K_FOREVER);
	k_sem_take(&mutex, K_FOREVER);
	
	buffer[in] = v;
	in = (in + 1) % SIZE;
	
	k_sem_give(&mutex);
	k_sem_give(&cons);
}

int get_from_buffer(){
	int v;
	k_sem_take(&cons, K_FOREVER);
	k_sem_take(&mutex, K_FOREVER);
	
	v = buffer[out];
	out = (out + 1) % SIZE;
	
	k_sem_give(&mutex);
	k_sem_give(&prod);
	return v;
}

void produtor(void *id, void *unused1, void *unused2){
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	int my_id = 0;

	while(1){
		int value = get_random_int(my_id, 1000);
		set_to_buffer(value);
		printk("\nprodutor");
		k_sleep(1000);
	}
}

void consumidor(void *id, void *unused1, void *unused2){
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	int my_id = 1;
	int value;

	while(1){
		printk("\nconsumidor");
		value = get_from_buffer();
		printk("%d %d\n", my_id, value);
		k_sleep(2000);
	}
}

void main(void){

	k_sem_init(&prod, SIZE, SIZE);
	k_sem_init(&cons, 0, SIZE);
	k_sem_init(&mutex, 1, 1);
	
	k_thread_create(&tprod, stack_prod, STACKSIZE,
				produtor, NULL, NULL, NULL, 7,
				K_USER, K_FOREVER);
				
	k_thread_start(&tprod);
				
	k_thread_create(&tcons, stack_cons, STACKSIZE,
				consumidor, NULL, NULL, NULL, 7,
				K_USER, K_FOREVER);
	
	k_thread_start(&tcons);			
}
