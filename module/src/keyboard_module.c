void my_handler(char scancode, int pressed) { printk("Module: key event\n"); }

int init_module(void)
{
    register_keyboard_callback(my_handler);
    return 0;
}

void cleanup_module(void) { unregister_keyboard_callback(my_handler); }
