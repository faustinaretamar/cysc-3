#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/adc.h"0
#include "hardware/pwm.h"

/* Struct para los datos de temperatura */
typedef struct {
  float lm35;
  float pote;
} temperature_data_t;

/* Queue para comunicar los dos nucleos */
queue_t queue;

/* Main para el core 1 */
void core1_main() {
    /* configuracion del pwm */
gpio_set_function(0, GPIO_FUNC_PWM);
gpio_set_function(1, GPIO_FUNC_PWM);
uint slice_num = pwm_gpio_to_slice_num(0);
pwm_config config = pwm_get_default_config();
pwm_init(slice_num, &config, true); // activamos la pwm
    while(1) {
        /* Variable para recuperar el dato de la queue */
        temperature_data_t data; 
        /* Espera a que esten los datos para recibir */
        queue_remove_blocking(&queue, &data);
        printf ("La temperatura es %.2f\n", data.lm35 ); // mostramos la temperatura medida
        printf ("La temperatura deseada es %.2f\n", data.pote ); // mostramos la temperatura deseada
        float dif= data.pote-data.lm35; // calculamos la diferencia entre la temperatura deseada y la medida en el sensor 


        if (dif>0) { // diferencia negativa (lm35<pote)
            if (dif>10) {
            dif = 10;
            //la diferencia maxima que podemos manejar es de 10
            }
            uint16_t ancho = dif*65535/10; //calculamos en ancho de banda para llegar a la temperatura deseada (+)
            /* dejamos de enfriar y calentamos */
            pwm_set_gpio_level(0, ancho); 
            pwm_set_gpio_level(1, 0);
        }
        if (dif<0){ // diferencia negativa (lm35>pote)
            if (dif<-10) {
                dif = -10;
                //la diferencia maxima que podemos manejar es de -10
                }
            uint16_t ancho = -dif*65535/10; //calculamos en ancho de banda para llegar a la temperatura deseada (-)
            /* dejamos de calentar y enfriamos */
            pwm_set_gpio_level(0, 0);
            pwm_set_gpio_level(1, ancho);
        }
    }

}

/* Main para el core 0 */
int main() {
    stdio_init_all();
    /* Inicializa la cola para enviar un unico dato */
    queue_init(&queue, sizeof(temperature_data_t), 1);
    /* Inicializa el core 1 */
    multicore_launch_core1(core1_main);
    sleep_ms (500);
    adc_init();
    gpio_init(26); // elegimos los pines
    gpio_init(27);

    while(1) {
        /* Variable para enviar los datos */
        temperature_data_t data;
        adc_select_input(0);
        float vol1= adc_read();
        adc_select_input(1);
        float vol2= adc_read();
        float temp1= vol1/0.01;
        float temp2= vol2*35/3.3; // hacemos el calculo de la temperatura 
        data.lm35 = temp1; //definimos las variables 
        data.pote = temp2;
        sleep_ms (500);


        /* Cuando los datos estan listos, enviar por la queue */
        queue_add_blocking(&queue, &data);
    }
}