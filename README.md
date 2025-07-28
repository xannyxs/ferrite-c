# Ferrite

Ferrite is an experimental kernel written in C.

> This project is in early stage development. While it's exciting to experiment with, it's not yet ready for production use.

## Building from Source

You'll need:

- `nix-shell` for an isolated development environment
- QEMU for virtualization

```bash
# Clone the repository
git clone https://github.com/xannyxs/ferrite-c
cd ferrite-c

# Initiate nix-shell
nix-shell shell.nix

# Build the kernel
make

# Run in QEMU
make run
```

## Documentation

Comprehensive documentation is available in the `/docs` directory. To explore the documentation:

```bash
mdbook serve --open docs
```

The documentation provides insights into Ferrite's design philosophy and implementation details, which could be valuable for your own kernel development journey. Take this with a grain of salt, since I am not fimiliar with everything either.

## License

Ferrite is licensed under the GPL License. See [LICENSE](LICENSE) for details.
