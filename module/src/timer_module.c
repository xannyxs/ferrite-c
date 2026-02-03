static unsigned long last_query_time = 0;

static void on_timer_tick(unsigned long ticks)
{
    if (ticks % 100 == 0) {
        printk("[Time Module] Tick %u\n", ticks);
    }
}

int init_module(void)
{
    printk("Time module: initializing\n");
    register_timer_callback(on_timer_tick);

    return 0;
}

void cleanup_module(void)
{
    printk("Time module: total queries: %u\n", last_query_time);
    unregister_timer_callback(on_timer_tick);
}
