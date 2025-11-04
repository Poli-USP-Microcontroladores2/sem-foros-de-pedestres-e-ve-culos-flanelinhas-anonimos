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

// --- Botão no PTA1 ---
static const struct gpio_dt_spec botao = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpioa)),
    .pin = 1,
    .dt_flags = GPIO_INPUT | GPIO_PULL_UP,
};

// --- Saída PB1 para pedestres ---
#define SINAL_PED_NODE DT_NODELABEL(gpiob)
#define SINAL_PED_PIN 1
static const struct gpio_dt_spec sinal_ped = {
    .port = DEVICE_DT_GET(SINAL_PED_NODE),
    .pin = SINAL_PED_PIN,
    .dt_flags = GPIO_OUTPUT_INACTIVE,
};

// --- Tempos ---
#define VERDE_TIME    4000
#define AMARELO_TIME  1000
#define VERMELHO_TIME 4000

// --- Estados ---
enum EstadoSemaforo {
    VERDE = 0,
    AMARELO,
    VERMELHO
};

volatile enum EstadoSemaforo estado_atual = VERDE;
volatile bool pedido_pedestre = false;

// --- Variável de modo noturno ---
volatile bool modo_noturno = false; // altere para true (1) para testar

// --- Callback do botão ---
void botao_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (modo_noturno) return; // ignora no modo noturno

    if (estado_atual == VERDE) { 
        pedido_pedestre = true;
        printk("Botao pressionado: pedido de pedestre recebido.\n");
    } else {
        printk("Botao pressionado, mas semaforo nao esta verde. Ignorado.\n");
    }
}

static struct gpio_callback botao_cb_data;

// --- Thread principal ---
void thread_veiculos(void *a, void *b, void *c)
{
    while (1) {
        if (modo_noturno) {
            // ===== MODO NOTURNO =====
            printk("Modo noturno ativo (veiculos piscando amarelo)\n");
            gpio_pin_set_dt(&led_vermelho, 1);
            gpio_pin_set_dt(&led_verde, 1); // amarelo (mistura de ambos)
            gpio_pin_set_dt(&sinal_ped, 1); // pedestres vermelhos
            k_sleep(K_MSEC(1000));
            gpio_pin_set_dt(&led_vermelho, 0);
            gpio_pin_set_dt(&led_verde, 0);
            k_sleep(K_MSEC(1000));
            continue; // reitera no modo noturno
        }

        // ===== VERDE =====
        estado_atual = VERDE;
        gpio_pin_set_dt(&led_verde, 1);
        gpio_pin_set_dt(&led_vermelho, 0);
        gpio_pin_set_dt(&sinal_ped, 1); // pedestres vermelhos
        printk("Semaforo VEICULOS: VERDE\n");

        int elapsed = 0;
        while (elapsed < VERDE_TIME) {
            if (modo_noturno) break;
            if (pedido_pedestre) break;
            k_msleep(100);
            elapsed += 100;
        }
        pedido_pedestre = false;

        if (modo_noturno) continue; // volta ao topo

        // ===== AMARELO =====
        estado_atual = AMARELO;
        gpio_pin_set_dt(&led_verde, 1);
        gpio_pin_set_dt(&led_vermelho, 1);
        gpio_pin_set_dt(&sinal_ped, 1);
        printk("Semaforo VEICULOS: AMARELO\n");
        k_sleep(K_MSEC(AMARELO_TIME));

        if (modo_noturno) continue;

        // ===== VERMELHO =====
        estado_atual = VERMELHO;
        gpio_pin_set_dt(&led_verde, 0);
        gpio_pin_set_dt(&led_vermelho, 1);
        gpio_pin_set_dt(&sinal_ped, 0); // abre pedestres
        printk("Semaforo VEICULOS: VERMELHO — pedestres podem atravessar\n");
        k_sleep(K_MSEC(VERMELHO_TIME));

        if (modo_noturno) continue;

        // ===== Retorna ao VERDE =====
        gpio_pin_set_dt(&sinal_ped, 1);
        printk("Ciclo completo. Retornando ao VERDE.\n");
    }
}

K_THREAD_DEFINE(thread_veic_id, THREAD_STACK_SIZE, thread_veiculos, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

void main(void)
{
    printk("=== Semaforo VEICULOS iniciado ===\n");

    gpio_pin_configure_dt(&led_verde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_vermelho, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&sinal_ped, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&botao, GPIO_INPUT | GPIO_PULL_UP);

    gpio_init_callback(&botao_cb_data, botao_callback, BIT(botao.pin));
    gpio_add_callback(botao.port, &botao_cb_data);
    gpio_pin_interrupt_configure_dt(&botao, GPIO_INT_EDGE_TO_ACTIVE);

    printk("Sistema iniciado. Botao em PTA1.\n");
}