# Medidor de Luz (Luxímetro) com  Raspberry Pi Pico

Este projeto utiliza um Raspberry Pi Pico para criar um medidor de intensidade de luz digital (luxímetro). Ele lê os dados de um sensor de luz ambiente **BH1750** e exibe o valor em **Lux** em tempo real em um display **OLED SSD1306**.


---

## Funcionalidades

* **Medição de Luz Ambiente:** Utiliza o sensor BH1750 para obter leituras precisas da intensidade luminosa (iluminância).
* **Display em Tempo Real:** Mostra o valor atual em Lux em um display OLED I2C de 128x64 pixels.
* **Duplo Barramento I2C:** Emprega duas portas I2C distintas do Pico: `i2c0` para o sensor e `i2c1` para o display, demonstrando o uso de múltiplos periféricos.
* **Botão BOOTSEL:** Um botão de pressão dedicado no pino `GP6` permite entrar no modo de gravação USB sem precisar desconectar o cabo ou usar o botão da placa.

---



## Funcionamento do Código

* **`main()`:** A função principal inicializa o `stdio`, as duas portas I2C, o display e o sensor BH1750. Também configura a interrupção no pino `GP6` para a função BOOTSEL.
* **Loop Infinito:** Dentro do `while(1)`, o programa executa os seguintes passos a cada 500 milissegundos:
    1.  Chama `bh1750_read_measurement()` para ler o valor de iluminância do sensor.
    2.  Imprime o valor lido no console serial (útil para depuração).
    3.  Formata o valor em uma string (ex: "1500 Lux").
    4.  Desenha uma interface gráfica no buffer do display e adiciona a string com o valor de Lux.
    5.  Envia o buffer para o display, atualizando a tela com as novas informações.
* **`gpio_irq_handler()`:** Esta função de *callback* é acionada quando o botão no `GP6` é pressionado. Sua única tarefa é chamar `reset_usb_boot(0, 0)`, que reinicia o microcontrolador diretamente no modo de gravação.

