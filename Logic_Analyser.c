#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "data_analyser.pio.h"

#define transfers 450
uint32_t buffer[transfers];
const uint pin = 15;
const uint power = 14;
const float div = 125000000 / (5 * 38000); //clock speed of 38000 * 5

uint dma_data_chan;
uint sm;
bool data_ok = false;
bool trigger_level = false; //wait for pin to drive low

void setup_dma_chans(uint, PIO, uint, volatile void *);
void print_bin(uint32_t, bool);
void dma_handler();
int main()
{
    stdio_init_all();
    sleep_ms(10000);
    printf("Logic Analyser Test\n");
    sleep_ms(2000);

    printf("setting up power\n");
    gpio_init(power);
    gpio_set_dir(power, true); //set to output
    gpio_put(power, true);


    uint offset = pio_add_program(pio0, &main_program);
    sm = pio_claim_unused_sm(pio0, true);

    dma_data_chan = dma_claim_unused_channel(true);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    printf("DMA IRQ enabled?: %d\n", irq_is_enabled(DMA_IRQ_0));
    gpio_pull_up(pin);
    printf("GPIO #%d is pulled up?: %d\n", pin, gpio_is_pulled_up(pin));
    sleep_ms(1000);
    main_program_init(pio0, sm, offset, pin, trigger_level, div);
    setup_dma_chans(dma_data_chan, pio0, sm, &buffer);

    while (1)
    {
        if (data_ok) //unable to add to interupt loop as it seems to take too long
        {
            printf("Incoming data stream\n");
            for (int i = 0; i < transfers; i++)
            {
                // printf("Data at index %-5d -->:", i);
                print_bin(buffer[i], true);
            }
            data_ok = false;
            printf("Resetting logger\n");
            dma_channel_set_write_addr(dma_data_chan, &buffer, true);
        }
    }

    return 0;
}
void setup_dma_chans(uint channel, PIO pio, uint sm, volatile void *data)
{
    dma_channel_config c = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_RX0 + sm);

    dma_channel_set_irq0_enabled(channel, true); //enable generation of interupts
    dma_channel_configure(
        channel,
        &c,
        data,    //don't set write address until after triggering the handler
        &pio->rxf[sm], //read 
        transfers,
        true    //don't start until interupt is called
    );
}
void print_bin(uint32_t data, bool reverse_order)
{
    if (!reverse_order)
    {
        for (int i = 0; i < 32; i++)
        {
            printf("%s", (((data >> (31 - i)) & 1u) == 0)? "_": "-");
        }
    }
    if (reverse_order)
    {
        for (int i = 0; i < 32; i++)
        {
            printf("%s", (((data >> (i)) & 1u) == 0)? "_": "-"); 
        }
    }
    printf("\n");
}
void dma_handler()
{
    data_ok = true;
    dma_channel_acknowledge_irq0(dma_data_chan);
    pio_sm_exec(pio0, sm, pio_encode_wait_pin(trigger_level, 0));
    pio_sm_clear_fifos(pio0, sm);
}