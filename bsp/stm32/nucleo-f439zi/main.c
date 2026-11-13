#include "board.h"
#include "start.h"

static s_thread producer;
static s_thread consumer;
static s_sem heartbeat;
static s_uint8_t producer_stack[512] __attribute__((aligned(8)));
static s_uint8_t consumer_stack[768] __attribute__((aligned(8)));

static void produce(void)
{
    int on = 0;
    for (;;)
    {
        on = !on;
        board_led_write(BOARD_LD1, on);
        s_sem_release(&heartbeat);
        s_mdelay(500);
    }
}

static void consume(void)
{
    for (;;)
    {
        if (s_sem_take(&heartbeat, START_WAITING_FOREVER) == S_OK)
        {
            board_led_write(BOARD_LD2, 1);
            s_printf("heartbeat\r\n");
            s_mdelay(100);
            board_led_write(BOARD_LD2, 0);
        }
    }
}

int main(void)
{
    board_init();
    if (s_start_init() == S_OK &&
        s_sem_init(&heartbeat, 0, START_IPC_FLAG_FIFO) == S_OK &&
        s_thread_init(&consumer, consume, consumer_stack, sizeof(consumer_stack), 4, 10) == S_OK &&
        s_thread_init(&producer, produce, producer_stack, sizeof(producer_stack), 5, 10) == S_OK &&
        s_thread_startup(&consumer) == S_OK && s_thread_startup(&producer) == S_OK)
    {
        s_printf("NUCLEO-F439ZI, HCLK %d MHz\r\n", (int)(SystemCoreClock / 1000000UL));
        (void)s_irq_disable();
        board_tick_start();
        s_sched_start();
    }
    for (;;) {}
}
