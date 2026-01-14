#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/uart.h"
#include <stdio.h>
#include <math.h>
#include "hardware/watchdog.h"

#define SAMPLES_PER_CYCLE 256
#define SAMPLE_RATE 60 * SAMPLES_PER_CYCLE
#define CYCLES 3
#define TOTAL_SAMPLES (SAMPLES_PER_CYCLE * CYCLES)

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define TRIGGER_GPIO 16

volatile bool trigger_medida = false;

void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == TRIGGER_GPIO && (events & GPIO_IRQ_EDGE_RISE))
    {
        trigger_medida = true;
    }
}

const float WC = 2.0f * M_PI * 106.0f;
const float A = 2.0f * SAMPLE_RATE;
const float B = WC * sqrtf(2.0f);
const float C = WC * WC;
const float raizde2 = sqrtf(2.0f);

const float K1 = C / (A * A + A * B + C);
const float K2 = 2.0f * (C - A * A) / (A * A + A * B + C);
const float K3 = (A * A + C - A * B) / (A * A + A * B + C);

const float hc[16] = {(4.0 / 32.0) * cos(2.0 * M_PI * 1.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 2.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 3.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 4.0 / 32.0),
                      (4.0 / 32.0) * cos(2.0 * M_PI * 5.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 6.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 7.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 8.0 / 32.0),
                      (4.0 / 32.0) * cos(2.0 * M_PI * 9.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 10.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 11.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 12.0 / 32.0),
                      (4.0 / 32.0) * cos(2.0 * M_PI * 13.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 14.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 15.0 / 32.0), (4.0 / 32.0) * cos(2.0 * M_PI * 16.0 / 32.0)};

const float hs[16] = {(4.0 / 32.0) * sin(2.0 * M_PI * 1.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 2.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 3.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 4.0 / 32.0),
                      (4.0 / 32.0) * sin(2.0 * M_PI * 5.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 6.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 7.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 8.0 / 32.0),
                      (4.0 / 32.0) * sin(2.0 * M_PI * 9.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 10.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 11.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 12.0 / 32.0),
                      (4.0 / 32.0) * sin(2.0 * M_PI * 13.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 14.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 15.0 / 32.0), (4.0 / 32.0) * sin(2.0 * M_PI * 16.0 / 32.0)};

#define WDT_TIMEOUT_MS 1000 // timeout
#define FAIL_GPIO 17        // GPIO pra simular travamento
#define LED_RED 13          // pino do led vermelho

// Função auxiliar: feed seguro (permite simular falha)
static inline void wdt_feed_if_ok(void)
{
    // Se FAIL_GPIO estiver em nível alto, simula travamento -> não alimenta o sistema
    if (!gpio_get(FAIL_GPIO))
    {
        watchdog_update();
    }
}

int main()
{
    bool hardware_failure = false;
    stdio_init_all();

    // Diagnóstico do reset
    if (watchdog_caused_reboot())
    {
        hardware_failure = true;
        printf("Reboot: watchdog\n");
    }
    else
    {
        printf("Reboot: power-on / reset pin\n");
    }

    sleep_ms(2000); // Aguarda inicialização da porta serial

    // Configura pino de falha
    gpio_init(FAIL_GPIO);
    gpio_set_dir(FAIL_GPIO, GPIO_IN);
    gpio_pull_down(FAIL_GPIO);

    // Configura pino do led vermelho
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    // Indica falha logo no boot
    if (hardware_failure)
    {
        gpio_put(LED_RED, 1); // liga LED imediatamente
    }
    else
    {
        gpio_put(LED_RED, 0);
    }

    // Habilita watchdog
    watchdog_enable(WDT_TIMEOUT_MS, 1 /* pause_on_debug */);

    // --- Trigger no GPIO 15 ---
    gpio_init(TRIGGER_GPIO);
    gpio_set_dir(TRIGGER_GPIO, GPIO_IN);
    gpio_pull_down(TRIGGER_GPIO);

    gpio_set_irq_enabled_with_callback(
        TRIGGER_GPIO,
        GPIO_IRQ_EDGE_RISE,
        true,
        &gpio_callback);

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(26); // ADC0 no GPIO26
    adc_gpio_init(27); // ADC1 no GPIO27
    adc_gpio_init(28); // ADC2 no GPIO28

    // Inicializa comunicação UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    char buffer_uart[80];

    // Configura clock do ADC para 15.36kHz * 3 canais = 46.08MHz
    adc_set_clkdiv(48000000.0 / (SAMPLE_RATE * 3));

    // Configura modo round-robin: alterna entre ch0 e ch1
    adc_select_input(0);
    adc_set_round_robin((1 << 0) | (1 << 1) | (1 << 2)); // canais 0, 1 e 2
    adc_fifo_setup(
        true, // habilita FIFO
        true, // habilita req de DMA
        1,    // DREQ a cada amostra
        false,
        false);
    adc_fifo_drain(); // Limpa FIFO

    // DMA channel
    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_ADC);

    // Buffer para leituras intercaladas (ch0, ch1, ch2, ch0, ch1, ch2, ...)
    static uint16_t raw_data[TOTAL_SAMPLES * 3];

    // Inicia transferência DMA
    dma_channel_configure(
        dma_chan,
        &c,
        raw_data,          // destino
        &adc_hw->fifo,     // fonte
        TOTAL_SAMPLES * 3, // 3 canais intercalados
        false);

    sleep_ms(100);

    wdt_feed_if_ok();

    int N = TOTAL_SAMPLES / CYCLES;
    trigger_medida = false;
    while (1)
    {
        while (!trigger_medida)
        {
            tight_loop_contents(); // dica pro SDK, não adiciona atraso perceptível
            wdt_feed_if_ok();
        }

        trigger_medida = false;

        // reconfigura o número de transferências
        dma_channel_set_trans_count(dma_chan, TOTAL_SAMPLES * 3, false);
        // aponta o buffer e dispara
        dma_channel_set_write_addr(dma_chan, raw_data, true);

        adc_run(true);
        dma_channel_wait_for_finish_blocking(dma_chan);
        adc_run(false);
        adc_fifo_drain();

        uint count1 = 0, count2 = 0, count3 = 0;
        float soma_va = 0, soma_vb = 0, soma_vc = 0;

        float Y_cva = 0, Y_sva = 0, Y_cvb = 0, Y_svb = 0, Y_cvc = 0, Y_svc = 0;

        // estados do filtro tensão va
        float y_va1 = 0, y_va2 = 0;
        float va_ant1 = 0, va_ant2 = 0;

        // estados do filtro tensão vb
        float y_vb1 = 0, y_vb2 = 0;
        float vb_ant1 = 0, vb_ant2 = 0;

        // estados do filtro tensão vc
        float y_vc1 = 0, y_vc2 = 0;
        float vc_ant1 = 0, vc_ant2 = 0;

        // Primeiro calcula os offsets médios
        float soma_ch0 = 0, soma_ch1 = 0, soma_ch2 = 0;
        for (int i = 0; i < TOTAL_SAMPLES; i++)
        {
            soma_ch0 += raw_data[3 * i];     // canal tensão vc
            soma_ch1 += raw_data[3 * i + 1]; // canal tensão vb
            soma_ch2 += raw_data[3 * i + 2]; // canal tensão va
        }
        float offset_va = soma_ch2 / TOTAL_SAMPLES;
        float offset_vb = soma_ch1 / TOTAL_SAMPLES;
        float offset_vc = soma_ch0 / TOTAL_SAMPLES;

        wdt_feed_if_ok();

        for (int i = 0; i < TOTAL_SAMPLES; i++)
        {
            // pega valores intercalados
            float leitura_va = ((float)raw_data[3 * i + 2] - offset_va) * 0.90614530136f;
            float leitura_vb = ((float)raw_data[3 * i + 1] - offset_vb) * 0.90614530136f;
            float leitura_vc = ((float)raw_data[3 * i] - offset_vc) * 0.90614530136f;

            // filtro va
            float y_va = -K2 * y_va1 - K3 * y_va2 + K1 * leitura_va + 2 * K1 * va_ant1 + K1 * va_ant2;
            va_ant2 = va_ant1;
            va_ant1 = leitura_va;
            y_va2 = y_va1;
            y_va1 = y_va;

            // filtro vb
            float y_vb = -K2 * y_vb1 - K3 * y_vb2 + K1 * leitura_vb + 2 * K1 * vb_ant1 + K1 * vb_ant2;
            vb_ant2 = vb_ant1;
            vb_ant1 = leitura_vb;
            y_vb2 = y_vb1;
            y_vb1 = y_vb;

            // filtro vc
            float y_vc = -K2 * y_vc1 - K3 * y_vc2 + K1 * leitura_vc + 2 * K1 * vc_ant1 + K1 * vc_ant2;
            vc_ant2 = vc_ant1;
            vc_ant1 = leitura_vc;
            y_vc2 = y_vc1;
            y_vc1 = y_vc;

            // acumula só último ciclo
            if (i >= (CYCLES - 1) * N)
            {
                count1++;
                if (count1 >= 8 && count2 < 16)
                {
                    Y_cva += y_va * hc[15 - count2];
                    Y_sva += y_va * hs[15 - count2];
                    Y_cvb += y_vb * hc[15 - count2];
                    Y_svb += y_vb * hs[15 - count2];
                    Y_cvc += y_vc * hc[15 - count2];
                    Y_svc += y_vc * hs[15 - count2];
                    count1 = 0;
                    count2++;
                }
                soma_va += y_va * y_va;
                soma_vb += y_vb * y_vb;
                soma_vc += y_vc * y_vc;
                // soma_i += z*z;
                // soma_p += y*z;
                // printf("%.2f,%.2f,%.2f,%.2f;", y, z, leitura_tensao, leitura_corrente);
            }
        }

        float angulo_va = atan2f(Y_sva, Y_cva);
        float angulo_vb = atan2f(Y_svb, Y_cvb);
        float angulo_vc = atan2f(Y_svc, Y_cvc);

        // float modulo_va = sqrtf(Y_cva*Y_cva + Y_sva*Y_sva);
        // float modulo_vb = sqrtf(Y_cvb*Y_cvb + Y_svb*Y_svb);
        // float modulo_vc = sqrtf(Y_cvc*Y_cvc + Y_svc*Y_svc);

        // float Va_rms = modulo_va/raizde2;
        // float Vb_rms = modulo_vb/raizde2;
        // float Vc_rms = modulo_vc/raizde2;

        float Va_rms = sqrtf(soma_va / N);
        float Vb_rms = sqrtf(soma_vb / N);
        float Vc_rms = sqrtf(soma_vc / N);

        // Formata e envia os dados via UART
        snprintf(buffer_uart, sizeof(buffer_uart), "Va%.4f Vb%.4f Vc%.4f pA%.4f pB%.4f pC%.4f;", Va_rms, Vb_rms, Vc_rms, angulo_va, angulo_vb, angulo_vc);
        uart_puts(UART_ID, buffer_uart);

        wdt_feed_if_ok();
        printf("\n\n\n");
        printf("Defasagem: %.2f, %.2f, %.2f graus\n", angulo_va * (180.0f / M_PI), angulo_vb * (180.0f / M_PI), angulo_vc * (180.0f / M_PI));
        // printf("Modulo Tensao: %.2f V, %.2f V, %.2f V\n", modulo_va, modulo_vb, modulo_vc);
        printf("Tensao RMS: %.2f V, %.2f V, %.2f V\n", Va_rms, Vb_rms, Vc_rms);
    }
}
