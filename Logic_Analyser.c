#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "data_analyser.pio.h"
#include "hardware/pwm.h"

#define captureSize 450
#define capturePin 11
#define signal 12

void dmaHandler();
static inline void setupDma(PIO, uint);
static inline void setupPIO(PIO, uint, uint, uint, bool, float);

uint32_t buffer[captureSize];
uint dmaChan;
PIO pio;
uint sm;
const bool invertBools = false;
const bool triggerLevel = true;
const bool captureOnce = false;

int main()
{
    stdio_init_all();
    dmaChan = dma_claim_unused_channel(true);
    // float clkdiv = 125000000 / (38000 * 5);

    float clkdiv = 2500;

    irq_set_exclusive_handler(DMA_IRQ_0, dmaHandler);
    irq_set_enabled(DMA_IRQ_0, true);
    printf("%d\n", irq_is_enabled(DMA_IRQ_0));

    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    
    // uint capturePin = 13;
    // uint vS = 14;

    uint offset = pio_add_program(pio, &main_program);

    // setup power to sensor
    // gpio_init(vS);
    // gpio_set_dir(vS, true);
    // gpio_put(vS, true);
    // for interfacing with sensor

    // gpio_pull_up(capturePin);
    gpio_pull_down(capturePin);


    setupDma(pio, sm);
    setupPIO(pio, sm, offset, capturePin, triggerLevel, clkdiv);

    gpio_set_function(signal, GPIO_FUNC_PWM);
    pwm_config c = pwm_get_default_config();
    pwm_config_set_wrap(&c, 49999);
    pwm_config_set_clkdiv(&c, 50);
    pwm_init(pwm_gpio_to_slice_num(signal), &c, true);
    pwm_set_gpio_level(signal, 3250);


    while (true)
    {
        // printf("%u\n", pio_sm_get_blocking(pio, sm));
        tight_loop_contents();

        // printf("%d\n", dma_channel_is_busy(dmaChan));
    }
    return 0;
}

void dmaHandler()
{
    printf("Received, proccessing ready\n");
    uint8_t start = buffer[0] & 1u;
    bool state = (bool) ((buffer[0] & 1u) == 1); //lsb starts getting proccessed first
    state = (invertBools) ? !state: state;
    bool currentState;
    uint32_t stateWidth = 0;
    for (int i = 0; i < captureSize; i++)
    {
        for (int j = 0; j < 32; j++) //iterate from lsb of data
        {
            currentState = ((bool) ((buffer[i] >> j) & 1u));
            if (invertBools)
            {
                currentState = !currentState;
            }
            if (currentState == state)
            {
                stateWidth += 1;
            }
            if (currentState != state)
            {
                printf("State: %s, Width: %d\n", state ? "True": "False", stateWidth);

                stateWidth = 1;
                state = !state;
            }
        }
    }
    if (stateWidth != 1)
    {
        printf("State: %s, Width: %d\n", state ? "True": "False", stateWidth);
    }
    // printf("Transfer done\n");


    dma_channel_acknowledge_irq0(dmaChan);
    pio_sm_exec(pio, sm, pio_encode_wait_pin(triggerLevel, 0));
    pio_sm_clear_fifos(pio, sm);
    if (!captureOnce) //don't restart
    {
        dma_channel_set_write_addr(dmaChan, &buffer, triggerLevel);
    }
    return;
}
static inline void setupPIO(PIO pio, uint sm, uint offset, uint pin, bool trigger, float clkdiv)
{
    pio_sm_config c = main_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin);
    sm_config_set_in_shift(&c, true, true, 32);  //set autopush to true
    sm_config_set_clkdiv(&c, clkdiv);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_exec(pio, sm, pio_encode_wait_pin(trigger, 0));
    pio_sm_set_enabled(pio, sm, true);

}

static inline void setupDma(PIO pio, uint sm)
{
    dma_channel_config dc = dma_channel_get_default_config(dmaChan);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_dreq(&dc, pio_get_dreq(pio, sm, false));
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, true);

    dma_channel_set_irq0_enabled(dmaChan, true);
    dma_channel_configure(dmaChan,
        &dc,
        &buffer,
        &pio->rxf[sm],
        captureSize,
        true
    );

}

// % c-sdk {
//     static inline void initCapture(Pio pio, uint sm, uint offset, uint pin, float div)
//     {
        
//     }

// %}