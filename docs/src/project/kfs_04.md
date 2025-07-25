# KFS 04 - Interrupts

## Introduction

Interrupts are a critical part of any operating system, serving as a key mechanism for handling everything from hardware input to critical CPU errors. They allow the OS to respond to events asynchronously, ensuring that unexpected exceptions don't become fatal system crashes and that devices like the keyboard can communicate with the CPU efficiently. This post details the process of implementing a complete interrupt handling system.

## Goals

The project goals were as follows:

- **Create and Register an Interrupt Descriptor Table (IDT):** The foundational step was to properly define, populate, and load an IDT so the CPU knows where to find our handlers.

- **Implement a Signal-Callback System:** Design a kernel API that can associate interrupt signals with specific callback functions.

- **Develop an Interrupt Scheduling Interface:** To keep the time spent in an actual interrupt minimal, the goal was to create a system to schedule the main workload of a handler to be run outside of the interrupt context.

- **Implement Pre-Panic Cleanup:** For security and stability, create an interface to clean general-purpose registers before a system panic or halt.

- **Implement Stack Saving on Panic:** To aid in debugging, the system needed to save the stack state before a panic, allowing for post-mortem analysis of what caused the error.

- **Implement Keyboard Handling:** With the core interrupt system in place, the final goal was to use it to handle input from the keyboard.

## Technical Approach & Implementation

My approach was as straightforward as it could be. I began by initializing the IDT, creating the necessary exception handlers, and testing them. Programming the IDT is quite abstract since much of the logic is already baked into the CPU, leaving little room for creative implementation.

I implemented the IDT as follows:

```c
// idt.c

typedef struct {
    interrupt_handler_e type;
    union {
        // Some functions receive an error_code to help identify the error.
        interrupt_handler regular;
        interrupt_handler_with_error with_error_code;
    } handler;
} interrupt_handler_entry_t;

const interrupt_handler_entry_t INTERRUPT_HANDLERS[17] = {
    {REGULAR, .handler.regular = divide_by_zero_handler},
    ... // The other functions
};

interrupt_descriptor_t idt_entries[IDT_ENTRY_COUNT];
descriptor_pointer_t idt_ptr;

void idt_set_gate(uint32_t num, uint32_t handler) {
    ...
}

void idt_init(void) {
    for (int32_t i = 0; i < 17; i += 1) {
        uint32_t handler = 0;

        if (INTERRUPT_HANDLERS[i].type == REGULAR) {
            handler = (uint32_t)INTERRUPT_HANDLERS[i].handler.regular;
        } else if (INTERRUPT_HANDLERS[i].type == WITH_ERROR_CODE) {
            handler = (uint32_t)INTERRUPT_HANDLERS[i].handler.with_error_code;
        }

        // Pass the index & address of the function to set it correctly in the IDT.
        idt_set_gate(i, handler);
    }

    // Set gates for the first 32 exceptions.
    // 0x08 is the kernel code segment selector. 0x8E are the flags for an interrupt gate.
    idt_ptr.limit = sizeof(entry_t) * IDT_ENTRY_COUNT - 1;
    idt_ptr.base = (uint32_t)&idt_entries;

    // Load the IDT using the lidt assembly instruction.
    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
```

```c
// exceptions.c

__attribute__((target("general-regs-only"), interrupt)) void
divide_by_zero_handler(registers_t *regs) {
    // The 'cs' register is on the stack as part of the interrupt frame.
    // We can inspect it to see if the fault was in kernel-mode (CPL=0) or user-mode (CPL=3).
    // This requires a more complex reading of the interrupt frame.
    // For now, we assume a kernel fault is a panic.
    if ((regs->cs & 3) == 0) {
        panic(regs, "Division by Zero");
    }

    __asm__ volatile("cli; hlt");
}
```

The core of the logic revolves around two key variables: `idt_entries` and `idt_ptr`. The `idt_entries` array is the table itself, which will hold all 256 vectors. The `idt_ptr` is the structure we pass to the CPU, containing the base address and limit (size) of the table, so the processor knows exactly where to find it.

In the `idt_init()` function, we loop through our predefined exception handlers. While you could hardcode each `idt_set_gate()` call, a loop makes the code cleaner. This loop retrieves the memory address for each handler function and calls `idt_set_gate()` to correctly populate the entry in the idt_entries table.

The final step is the `lidt` assembly instruction. This tells the CPU to load our `idt_ptr`, making our new Interrupt Descriptor Table active. From this point on, the CPU will use our table to find the correct handler for any interrupt or exception that occurs.

When an interrupt happens, the CPU needs to stop its current work and jump to the handler, but it must be able to return and resume its work later. The `__attribute__((interrupt))` tells the compiler to automatically add the necessary assembly code to save the machine's state before your C code runs and restore it after. This is why interrupts should be as fast as possible; while a handler is running, the rest of the system is paused. For frequent events like keyboard presses, a common strategy is for the handler to do the bare minimum—like adding a key press to a queue—and letting a separate, lower-priority task scheduler process it later.

Once the IDT was set, I added a task scheduler. Inside an interrupt handler, I would add a task to the task_scheduler, which would add it to a queue. The main kernel loop then calls run_scheduled_tasks() to trigger the actual work of the interrupt. This is a great way to avoid staying too long in the interrupt itself. The shorter the interrupt, the faster and more responsive your kernel will be.

The `panic()` function is designed to terminate everything gracefully when a fatal, unrecoverable error occurs. When panicking, it's important to not only print an error message but also to dump the register state for debugging and then clean them for security. Keep the printing to a minimum, since you cannot rely on the stability of any system services at this point.

After that was all set, I was able to set up the keyboard. This required communicating with the `8259 PIC` (Programmable Interrupt Controller), which manages hardware interrupts. The keyboard sends a signal to the PIC, which then interrupts the CPU. I made use of my task scheduler to queue up keyboard presses to spend as little time as possible in the interrupt. It looks something like this:

```c
// exception.c

__attribute__((target("general-regs-only"), interrupt)) void
keyboard_handler(registers_t *regs) {
    // Read the scancode from the keyboard's data port.
    int32_t scancode = inb(KEYBOARD_DATA_PORT);
    setscancode(scancode); // Store the scancode for processing.

    // Schedule the main keyboard logic to run outside the interrupt.
    schedule_task(keyboard_input);

    // Send End-of-Interrupt (EOI) signal to the PIC.
    pic_send_eoi(1);
}
```

```c
// Kernel.c

while (true) {
    // In the main kernel loop, execute any tasks that have been scheduled.
    run_scheduled_tasks();

    // Halt the CPU until the next interrupt occurs to save power.
    __asm__ volatile("hlt");
}
```

To break it down, the `keyboard_handler()` first reads the `scancode` from the keyboard. It then schedules the real processing task and immediately sends an End-of-Interrupt (EOI) signal to the `PIC`, telling it we're done. Meanwhile, the main kernel while-loop continuously runs any scheduled tasks, ensuring that the heavy lifting happens outside of the critical interrupt context.

## Challenges

## Conclusion & Lesson Learned
