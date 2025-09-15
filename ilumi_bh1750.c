#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bh1750_light_sensor.h"
#include "ssd1306.h"
#include "font.h"
#include <math.h>
#include "hardware/pwm.h"

// Para o sensor de luz BH1750. Endereço 0x23
#define I2C_PORT i2c0 // i2c0 pinos 0 e 1, i2c1 pinos 2 e 3
#define I2C_SDA 0     // 0 ou 2
#define I2C_SCL 1     // 1 ou 3

// Display na I2C
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

// --- Definições dos Pinos ---
const uint SERVO_PIN = 18;
const uint JOYSTICK_X_PIN = 26; // Usaremos apenas o eixo X (ADC0)

// --- Constantes do PWM para o Servo de Rotação Contínua ---
const uint32_t WRAP_VALUE = 20000;
const float CLK_DIV = 125.0f;

// Valores de pulso para o servo de rotação contínua
const uint16_t SERVO_MAX_REVERSE_PULSE = 500;  // Velocidade máxima, sentido anti-horário
const uint16_t SERVO_STOP_PULSE = 1500;        // Pulso para PARAR o motor
const uint16_t SERVO_MAX_FORWARD_PULSE = 2500; // Velocidade máxima, sentido horário

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}


//Mapeia os valores obtido do GY-302 para converter no pwm do servomotor
// Mapeia um valor de um intervalo para outro
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Fim do trecho para modo BOOTSEL com botão B

    stdio_init_all();

    // I2C do Display pode ser diferente dos sensores. Funcionando em 400Khz.
    i2c_init(I2C_PORT_DISP, 400 * 1000);

    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o I2C0
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o sensor de luz BH1750
    bh1750_power_on(I2C_PORT);

    // --- Inicialização do PWM para o Servo ---
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, WRAP_VALUE);
    pwm_config_set_clkdiv(&config, CLK_DIV);
    pwm_init(slice_num, &config, true);
    uint channel = pwm_gpio_to_channel(SERVO_PIN);
    // Começa com o servo parado
    pwm_set_gpio_level(SERVO_PIN, SERVO_STOP_PULSE);

    char str_lux[10]; // Buffer para armazenar a string

    bool cor = true;
    uint16_t pulse_width;
    while (1)
    {
        // Leitura do sensor de Luz BH1750
        uint16_t lux = bh1750_read_measurement(I2C_PORT);
        printf("Lux = %d\n", lux);

        sprintf(str_lux, "%d Lux", lux); // Converte o inteiro em string

        // cor = !cor;
        //  Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor);                            // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);        // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);             // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);             // Desenha uma linha
        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);   // Desenha uma string
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);    // Desenha uma string
        ssd1306_draw_string(&ssd, "Sensor  BH1750", 10, 28); // Desenha uma string
        ssd1306_line(&ssd, 63, 25, 63, 37, cor);             // Desenha uma linha vertical
        ssd1306_draw_string(&ssd, str_lux, 14, 41);          // Desenha uma string
        ssd1306_send_data(&ssd);                             // Atualiza o display

        // Verifica se o lux está dentro da zona morta definida previamente
        if (lux > 0 && lux < 200)
        {
            // envia o pulso de parada
            pulse_width = SERVO_STOP_PULSE;
        }
        else
        {
            // Se estiver fora da zona morta, mapeia a posição para a velocidade
            // A função map() cuidará de ambas as direções
            pulse_width = map(lux, 0, 1500, SERVO_MAX_REVERSE_PULSE, SERVO_MAX_FORWARD_PULSE);
        }

        // Calcula o ciclo de trabalho (duty cycle) para o PWM
        uint32_t duty_cycle = (pulse_width * (WRAP_VALUE + 1)) / 1000000;

        // Atualiza o ciclo de trabalho do PWM
        pwm_set_chan_level(slice_num, channel, duty_cycle);

         // Define a velocidade e direção do servo
        pwm_set_gpio_level(SERVO_PIN, pulse_width);
        sleep_ms(50);
    }

    return 0;
}