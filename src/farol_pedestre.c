#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

// --- Configuração de LEDs via DeviceTree ---
#define LED_VERDE_NODE DT_ALIAS(led0)
#define LED_VERM_NODE  DT_ALIAS(led2)

static const struct gpio_dt_spec led_verde = GPIO_DT_SPEC_GET(LED_VERDE_NODE, gpios);
static const struct gpio_dt_spec led_vermelho = GPIO_DT_SPEC_GET(LED_VERM_NODE, gpios);

// --- Configuração manual do botão em PTA1 ---
#define BOTAO_PORT   DEVICE_DT_GET(DT_NODELABEL(gpioa))
#define BOTAO_PIN    1

// --- Mutex para exclusão mútua ---
struct k_mutex led_mutex;

// --- Tempos em milissegundos ---
#define TEMPO_VERDE_MS     4000
#define TEMPO_VERMELHO_MS  2000
#define TEMPO_PISCAR_MS    1000

// --- Variáveis de controle ---
volatile bool modo_alerta = 0;
volatile bool estado_verde = 0;
volatile bool led_vermelho_aceso = 0;

// ----------------------------------------------------
// THREAD VERDE — acende LED verde por 4 segundos
// ----------------------------------------------------
void thread_verde(void *p1, void *p2, void *p3)
{
    while (1) {
        if (modo_alerta == 0 && estado_verde == 1) {
            k_mutex_lock(&led_mutex, K_FOREVER);

            gpio_pin_set_dt(&led_verde, 1);
            printk("LED VERDE ligado\n");
            k_msleep(TEMPO_VERDE_MS);
            gpio_pin_set_dt(&led_verde, 0);
            printk("LED VERDE desligado\n");

            k_mutex_unlock(&led_mutex);
            estado_verde = 0; // libera o vermelho
        } else {
            gpio_pin_set_dt(&led_verde, 0);
            k_msleep(100);
        }
    }
}

// ----------------------------------------------------
// THREAD VERMELHA — acende LED vermelho por 2 segundos
// ----------------------------------------------------
void thread_vermelha(void *p1, void *p2, void *p3)
{
    while (1) {
        if (modo_alerta == 0 && estado_verde == 0) {
            k_mutex_lock(&led_mutex, K_FOREVER);

            gpio_pin_set_dt(&led_vermelho, 1);
            led_vermelho_aceso = 1;
            printk("LED VERMELHO ligado\n");
            k_msleep(TEMPO_VERMELHO_MS);
            gpio_pin_set_dt(&led_vermelho, 0);
            led_vermelho_aceso = 0;
            printk("LED VERMELHO desligado\n");

            k_mutex_unlock(&led_mutex);
            estado_verde = 1; // depois do vermelho, vai para verde
        } 
        else if (modo_alerta == 1) {
            gpio_pin_set_dt(&led_vermelho, 1);
            printk("[ALERTA] LED VERMELHO piscando (aceso)\n");
            k_msleep(TEMPO_PISCAR_MS);
            gpio_pin_set_dt(&led_vermelho, 0);
            printk("[ALERTA] LED VERMELHO piscando (apagado)\n");
            k_msleep(TEMPO_PISCAR_MS);
        } 
        else {
            k_msleep(100);
        }
    }
}

// ----------------------------------------------------
// THREAD DO BOTÃO — só age se LED vermelho estiver aceso
// ----------------------------------------------------
void thread_botao(void *p1, void *p2, void *p3)
{
    int estado_anterior = 1; // Com PULL-UP: solto = 1, pressionado = 0

    while (1) {
        int estado_atual = gpio_pin_get(BOTAO_PORT, BOTAO_PIN);

        if (estado_atual < 0) {
            printk("Erro lendo o botão!\n");
            k_msleep(500);
            continue;
        }

        // Detecta pressão (borda de descida)
        if (estado_anterior == 1 && estado_atual == 0) {
            if (modo_alerta == 0 && led_vermelho_aceso == 1) {
                printk("Botão pressionado durante LED VERMELHO!\n");
                printk("Aguardando 1 segundo antes de mudar para verde...\n");
                k_msleep(1000);
                estado_verde = 1;
                printk("Transição para estado VERDE.\n");
            } else if (led_vermelho_aceso == 0) {
                printk("Botão pressionado, mas LED vermelho está apagado (ignorado).\n");
            }

            // debounce simples
            k_msleep(300);
        }

        estado_anterior = estado_atual;
        k_msleep(50);
    }
}

// ----------------------------------------------------
// Definição das threads
// ----------------------------------------------------
K_THREAD_DEFINE(thread_verde_id, 512, thread_verde, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_vermelha_id, 512, thread_vermelha, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_botao_id, 512, thread_botao, NULL, NULL, NULL, 4, 0, 0);

// ----------------------------------------------------
// Função principal
// ----------------------------------------------------
void main(void)
{
    if (!device_is_ready(led_verde.port) || !device_is_ready(led_vermelho.port)) {
        printk("Erro: LEDs não estão prontos!\n");
        return;
    }
    if (!device_is_ready(BOTAO_PORT)) {
        printk("Erro: GPIOA (botão) não está pronta!\n");
        return;
    }

    gpio_pin_configure_dt(&led_verde, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_vermelho, GPIO_OUTPUT_INACTIVE);

    // PTA1 como entrada com pull-up interno
    gpio_pin_configure(BOTAO_PORT, BOTAO_PIN, GPIO_INPUT | GPIO_PULL_UP);
    printk("Botão configurado em GPIOA pin %d (PTA1)\n", BOTAO_PIN);

    k_mutex_init(&led_mutex);

    printk("=== Controle de LEDs + Botão PTA1 ===\n");
    printk("→ O botão só funciona quando o LED vermelho está aceso.\n");
    printk("→ Após apertar, espera 1s e troca para verde.\n");

    estado_verde = 0; // começa com vermelho
}
