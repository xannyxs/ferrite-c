int init_module(void)
{
    printk("Hello from module!\n");
    return 0;
}

void cleanup_module(void) { printk("Goodbye from module!\n"); }
