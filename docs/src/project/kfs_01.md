# KFS 01 - Grub, boot & screen

## Introduction

Our first `KFS` project! I was quite nervous myself at first looking at this. My previous experience with kernels was limited to Linux from Scratch, which barely scratches the surface compared to KFS.

For this project, I chose the C Programming Language. I simply already have a few years of experience in C/C++ & Assembly, which will definitly be useful.

## Goals

- A kernel that can boot via Grub
- An `ASM` bootable base
- A basic Kernel Library
- Show "42" on screen

## Technical Approach & Implimentation

My approach was quite straightforward for this project. Read, read & READ! I primarily started reading [OSDev](https://wiki.osdev.org/Expanded_Main_Page). They have a straightforward tutorial for OS booting in `C/ASM`. With ease, I was able to make a system bootable.

[Philipp Oppermann's blog](https://os.phil-opp.com/) helped me a lot! The blog is focused on developing a Rust Kernel, but it gave me more insight on how to setup a C Language environment. His writing style is to me more understandable compared to OSDev.

After that I noticed _Mr. Oppermann_ having a second tutorial on VGA; how to set it up and print to it, which is one of the requirements. After setting that all up, I just had to put the dots on the i, and cross the t's.

I added a `nix-shell`. `nix-shell` creates an interactive shell based on a Nix expression.
It makes sure you get less of the _"It works on my machine"_ & it is also great if you switch often from different devices. Nix as a language on the otherhand is quite unintutive. Their documentation is known to be a sluggish, because of that I had a very hard time finding out how to cross-compile `gcc` using `nix-shell`.

## Challenges

The hardest challange of this project was understanding the `nix-shell`, because the documentation of Nix is quite limited. It was just trying a lot of things until it worked.

## Conclusion & Lesson Learned

In the end, it went much smoother than expected. There were plenty of tutorials and understandable documentation to get me through the first project.

I am still happy with my choice to use `nix-shell`. It will definitely avoid headaches in the future.
