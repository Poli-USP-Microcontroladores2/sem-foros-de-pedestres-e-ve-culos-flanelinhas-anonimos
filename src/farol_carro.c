#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdbool.h>

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 5

// ------------------ Mutex ------------------
struct k_mutex semaforo_mutex;

// ------------------ Tempos (ms) ------------------
#define VERDE_TIME 3000
#define AMARELO_TIME 1000
#define VERMELHO_TIME 4000
#define NOTURNO_TIME 1000

// ------------------ LEDs via DeviceTree ------------------
#define LED_VERDE_NODE DT_ALIAS(led0)
#define LED_VERMELHO_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec led_verde = GPIO_DT_SPEC_GET(LED_VERDE_NODE, gpios);
static const struct gpio_dt_spec led_vermelho = GPIO_DT_SPEC_GET(LED_VERMELHO_NODE, gpios);

// ------------------ Botão no PTA1 ------------------
static const struct gpio_dt_spec botao = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpioa)),
    .pin = 1,
    .dt_flags = GPIO_INPUT | GPIO_PULL_UP,
};

// ------------------ Variáveis globais ------------------
volatile bool modo_noturno = false; // Defina manualmente no código
volatile bool emergencia = false;   // True quando o botão é pressionado
volatile int estado = 0;            // 0=VERDE,1=AMARELO,2=VERMELHO

// ------------------ Funções auxiliares ------------------
static void apagar_todos_leds(void)
{
    gpio_pin_set_dt(&led_verde, 0);
    gpio_pin_set_dt(&led_vermelho, 0);
}

static void leds_init(void)
{
    gpio_pin_configure_dt(&led_verde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_vermelho, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&botao, GPIO_INPUT | GPIO_PULL_UP);
}

// ------------------ Callback do botão ------------------
void botao_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    static int last_state = 1; // botão solto = 1 (pull-up)
    int current = gpio_pin_get_dt(&botao);

    if (current == 0 && last_state == 1) { // borda de descida
        printk("🚶 Botão do pedestre pressionado! Emergência ativada.\n");
        emergencia = true;
    }
    last_state = current;
}

static struct gpio_callback botao_cb_data;

// ------------------ Thread: Modo Normal ------------------
void thread_normal(void *a, void *b, void *c)
{
    while (1) {
        // Espera se modo noturno ou emergência estiver ativo
        if (modo_noturno || emergencia) {
            k_sleep(K_MSEC(100));
            continue;
        }

        k_mutex_lock(&semaforo_mutex, K_FOREVER);
        apagar_todos_leds();

        // Verde
        gpio_pin_set_dt(&led_verde, 1);
        printk("🚗 Semáforo VERDE\n");
        int elapsed = 0;
        while (elapsed < VERDE_TIME) {
            if (emergencia) break; // interrompe para emergência
            k_sleep(K_MSEC(100));
            elapsed += 100;
        }
        gpio_pin_set_dt(&led_verde, 0);

        if (emergencia) {
            k_mutex_unlock(&semaforo_mutex);
            continue;
        }

        // Amarelo (verde + vermelho)
        gpio_pin_set_dt(&led_verde, 1);
        gpio_pin_set_dt(&led_vermelho, 1);
        printk("⚠️  Semáforo AMARELO (transição)\n");
        k_sleep(K_MSEC(AMARELO_TIME));
        apagar_todos_leds();

        // Vermelho
        gpio_pin_set_dt(&led_vermelho, 1);
        printk("🛑 Semáforo VERMELHO\n");
        k_sleep(K_MSEC(VERMELHO_TIME));
        apagar_todos_leds();

        k_mutex_unlock(&semaforo_mutex);
    }
}

// ------------------ Thread: Emergência (Pedestre) ------------------
void thread_emergencia(void *a, void *b, void *c)
{
    while (1) {
        if (emergencia) {
            k_mutex_lock(&semaforo_mutex, K_FOREVER);

            // Amarelo (verde+vermelho)
            apagar_todos_leds();
            gpio_pin_set_dt(&led_verde, 1);
            gpio_pin_set_dt(&led_vermelho, 1);
            printk("🚸 Emergência: AMARELO (alerta pedestre)\n");
            k_sleep(K_MSEC(AMARELO_TIME));

            // Vermelho
            apagar_todos_leds();
            gpio_pin_set_dt(&led_vermelho, 1);
            printk("🚦 Emergência: VERMELHO (pedestre atravessando)\n");
            k_sleep(K_MSEC(VERMELHO_TIME));

            // Retorna ao ciclo normal
            emergencia = false;
            apagar_todos_leds();
            printk("✅ Emergência finalizada, voltando ao modo normal\n");

            k_mutex_unlock(&semaforo_mutex);
        }
        k_sleep(K_MSEC(100));
    }
}

// ------------------ Thread: Modo Noturno ------------------
void thread_noturno(void *a, void *b, void *c)
{
    while (1) {
        if (modo_noturno) {
            k_mutex_lock(&semaforo_mutex, K_FOREVER);

            apagar_todos_leds();
            gpio_pin_set_dt(&led_verde, 1);
            gpio_pin_set_dt(&led_vermelho, 1);
            printk("🌙 Modo noturno: AMARELO piscando ON\n");
            k_sleep(K_MSEC(NOTURNO_TIME));

            apagar_todos_leds();
            printk("🌙 Modo noturno: AMARELO piscando OFF\n");
            k_sleep(K_MSEC(NOTURNO_TIME));

            k_mutex_unlock(&semaforo_mutex);
        } else {
            k_sleep(K_MSEC(200));
        }
    }
}

// ------------------ Definição das Threads ------------------
K_THREAD_DEFINE(thread_normal_id, THREAD_STACK_SIZE, thread_normal, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread_emergencia_id, THREAD_STACK_SIZE, thread_emergencia, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread_noturno_id, THREAD_STACK_SIZE, thread_noturno, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

// ------------------ Função Principal ------------------
void main(void)
{
    printk("=== Simulação de Semáforo Inteligente ===\n");

    leds_init();
    k_mutex_init(&semaforo_mutex);

    // Configura interrupção do botão
    gpio_init_callback(&botao_cb_data, botao_callback, BIT(botao.pin));
    gpio_add_callback(botao.port, &botao_cb_data);
    gpio_pin_interrupt_configure_dt(&botao, GPIO_INT_EDGE_BOTH);

    printk("Sistema iniciado. Aguardando eventos...\n");
    printk("Pressione o botão (PTA1) para simular pedestre.\n");
    printk("Defina 'modo_noturno = true' no código para ativar modo noturno.\n");
}
