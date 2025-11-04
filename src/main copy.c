#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdbool.h>

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5

// --- LEDs ---
#define LED_VERDE_NODE DT_ALIAS(led0)
#define LED_VERM_NODE  DT_ALIAS(led2)
static const struct gpio_dt_spec led_verde = GPIO_DT_SPEC_GET(LED_VERDE_NODE, gpios);
static const struct gpio_dt_spec led_vermelho = GPIO_DT_SPEC_GET(LED_VERM_NODE, gpios);

// --- Entrada PB1 dos veículos ---
#define SINAL_PED_NODE DT_NODELABEL(gpiob)
#define SINAL_PED_PIN 1
static const struct gpio_dt_spec sinal_ped = {
    .port = DEVICE_DT_GET(SINAL_PED_NODE),
    .pin = SINAL_PED_PIN,
    .dt_flags = GPIO_INPUT,
};

// --- Variável de modo noturno ---
volatile bool modo_noturno = false; // altere para true (1) para testar

void thread_pedestres(void *a, void *b, void *c)
{
    printk("Sistema de pedestres iniciado\n");

    while (1) {
        if (modo_noturno) {
            // ===== MODO NOTURNO =====
            printk("Modo noturno ativo (pedestres piscando vermelho)\n");
            gpio_pin_set_dt(&led_vermelho, 1);
            gpio_pin_set_dt(&led_verde, 0);
            k_sleep(K_MSEC(1000));
            gpio_pin_set_dt(&led_vermelho, 0);
            gpio_pin_set_dt(&led_verde, 0);
            k_sleep(K_MSEC(1000));
            continue;
        }

        int sinal = gpio_pin_get_dt(&sinal_ped);

        if (sinal == 0) {
            gpio_pin_set_dt(&led_vermelho, 0);
            gpio_pin_set_dt(&led_verde, 1);
            printk("Pedestres VERDE — pode atravessar\n");
        } else {
            gpio_pin_set_dt(&led_vermelho, 1);
            gpio_pin_set_dt(&led_verde, 0);
            printk("Pedestres VERMELHO — aguarde\n");
        }

        k_msleep(200);
    }
}

K_THREAD_DEFINE(thread_ped_id, THREAD_STACK_SIZE, thread_pedestres, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

void main(void)
{
    gpio_pin_configure_dt(&led_verde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_vermelho, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&sinal_ped, GPIO_INPUT);

    printk("Sistema de pedestres pronto.\n");
}