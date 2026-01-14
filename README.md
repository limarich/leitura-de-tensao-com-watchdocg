# Leitura de Tensão Trifásica com Monitoramento Watchdog (RP2040)

Este projeto implementa um sistema de leitura de tensão trifásica utilizando o microcontrolador RP2040 (Raspberry Pi Pico). O diferencial deste projeto é a implementação robusta de um **Watchdog Timer (WDT)** para garantir a confiabilidade do sistema e recuperação automática em caso de falhas.

## 🛡️ Sistema de Segurança Watchdog (Destaque)

O foco principal deste firmware é a demonstração e utilização do Watchdog para sistemas críticos de monitoramento.

### Como Funciona
*   **Monitoramento Contínuo:** O Watchdog é configurado com um timeout de **1000 ms**.
*   **Verificação de Saúde:** O loop principal do código deve "alimentar" o watchdog (`watchdog_update()`) periodicamente. Se o processador travar ou ficar preso em um loop infinito por mais de 1 segundo, o watchdog reinicia o microcontrolador automaticamente.
*   **Simulação de Falha:** Para fins de teste e validação, foi implementado um mecanismo de simulação de travamento:
    *   **Pino de Teste (GPIO 17):** Este pino é configurado como entrada com pull-down.
    *   **Ativação da Falha:** Se o GPIO 17 for colocado em nível ALTO (3.3V), o firmware **para propositalmente** de alimentar o watchdog.
    *   **Resultado:** O sistema irá reiniciar após 1 segundo.

### Diagnóstico de Reinicialização
Ao iniciar, o sistema verifica a causa do último reset:
*   **LED Vermelho (GPIO 13):** Acende se o reinício foi causado pelo **Watchdog** (indicando que houve uma falha anterior).
*   **LED Apagado:** Indica um reset normal (energia ligada ou botão de reset).

## ⚡ Funcionalidades de Medição
Além da segurança, o sistema realiza medições avançadas de sinais elétricos:
*   **Amostragem via DMA:** Utiliza DMA (Direct Memory Access) para capturar dados do ADC sem ocupar a CPU.
*   **Canais:** Lê 3 canais simultaneamente (ADC0, ADC1, ADC2) para as fases A, B e C.
*   **Processamento Digital:** Calcula Tensão RMS e Defasagem Angular entre as fases.
*   **Trigger Externo:** Inicia a medição baseada em um pulso no **GPIO 16**.

## 🔌 Pinagem (Hardware)

| Pino Pico | Função | Descrição |
|-----------|--------|-----------|
| **GPIO 26** | ADC 0 | Entrada Tensão Fase C |
| **GPIO 27** | ADC 1 | Entrada Tensão Fase B |
| **GPIO 28** | ADC 2 | Entrada Tensão Fase A |
| **GPIO 0** | UART TX | Saída de Dados Serial |
| **GPIO 1** | UART RX | Entrada de Dados Serial |
| **GPIO 16** | Trigger | Gatilho para início da medição (Edge Rise) |
| **GPIO 17** | **Simular Falha** | **HIGH (3.3V) = Travar Watchdog** |
| **GPIO 13** | **LED Status** | **Aceso = Resetado por Watchdog** |

## 🚀 Como Executar
### Demonstração em Vídeo
Assista ao vídeo de demonstração no YouTube: [https://youtu.be/X2b15_aPLxA](https://youtu.be/X2b15_aPLxA)

### Pré-requisitos
*   Raspberry Pi Pico SDK instalado.
*   CMake e Toolchain ARM GCC.

### Compilação
1.  Clone o repositório:
    ```bash
    git clone https://github.com/limarich/leitura-de-tensao-com-watchdocg.git
    cd leitura-de-tensao-com-watchdocg
    ```
2.  Crie a pasta de build e compile:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

### Teste do Watchdog
1.  Grave o firmware no Pico.
2.  O sistema funcionará normalmente (LED apagado).
3.  Conecte o **GPIO 17** ao **3.3V**.
4.  Aguarde 1 segundo. O Pico irá reiniciar.
5.  Ao voltar, o **LED Vermelho (GPIO 13)** estará ACESO, confirmando que o Watchdog atuou corretamente.
