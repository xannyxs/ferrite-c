=============================
KFS 01 - Grub, boot & screen
=============================

Introduction
------------

Our first ``KFS`` project! I was quite nervous myself at first looking at this. My previous experience with kernels was limited to Linux from Scratch, which barely scratches the surface compared to KFS.

For this project, I chose C as my programming language.
I do have a few years of experience in C/C++ & Assembly, which will definitly be useful.

Goals
-----

* A kernel that can boot via Grub
* An ``ASM`` bootable base
* A basic Kernel Library (strlen, memcpy, memcmp)
* Show "42" on screen

Technical Approach & Implimentation
-----------------------------------

My approach was quite straightforward for this project. Read, read & READ! I primarily started reading `OSDev <https://wiki.osdev.org/Expanded_Main_Page>`_. It offers good guidance on kernel development.

Before I would write any C code. I setup a ``nix-shell`` to ensure that all developors would work in a similair environment. ``nix-shell`` is also quite useful to easily cross-compile on the go.

After the setup was done, I started following OSDev's straightforward tutorial for OS booting in C. Having my own ``libc`` implementation made this phase quite smooth. With ease, I was able to make a system bootable.

After that, I had some basic knowledge on booting up a system via GRUB. `Philipp Oppermann's blog <https://os.phil-opp.com/>`_ helped me a lot! 

After that I noticed Mr. Oppermann having a second tutorial on VGA; how to set it up and print to it, which is one of the requirements. I finished it and mostly finished ``KFS_01``. I just had to put the dots on the i, and cross the t's.

Challenges
----------

There were not any noticable challanges for this project. The tutorials were quite straightforward & the project itself was not too long. Understanding the ``nix-shell`` syntax was the biggest challange for me.

Conclusion & Lesson Learned
---------------------------

In the end, it went much smoother than expected. There were plenty of tutorials and understandable documentation to get me through the first project.

I am quite happy with my choice to use ``nix-shell``. It will definitely avoid headaches in the future.
