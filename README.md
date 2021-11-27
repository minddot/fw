# fw
MindDot Firmware

RUST for Mindot
==============
Building Compiling Envronment
------------
* Install [Rustup](https://rustup.rs/).
* Install toolchain.
  - RiscV target - RiscV chips like ESP32-CXX are already supported in stock Rust.
  - Xtensa target - we need follow the [instruction steps](https://github.com/esp-rs/rust) for your operating system.
 
 Here, the new mindot will base on **RiscV** target chip.

* Build llvm Clang.
  You'll need the custom LLVM clang based on the Espressif LLVM fork for Rust STD support. we need follow [instruction steps](https://github.com/esp-rs/rust).
* Build APP.
  - Choose `esp` toolchain: 
    - `rustup default esp`
    - For RiscV target, we can use `nightly` directly:
      - `rustup install nightly`
      - `rustup default nightly`
  - Set Clang libary directory. `export LIBCLANG_PATH=<path to the Espressif Clang lib directory>`
  - Install linker. `cargo install ldproxy`
  - Build image. `cargo build`
* Flash
  - `cargo install espflash`
  - `espflash /dev/ttyUSB0 target/[xtensa-esp32-espidf|xtensa-esp32s2-espidf|riscv32imc-esp-espidf]/debug/rust-esp32-std-demo`
  - Replace **/dev/ttyUSB0** above with the USB port where you've connected the board.
* Monitor
  - `cargo install espmonitor`
  - `espmonitor /dev/ttyUSB0`
* For details, you can refer the [demo](https://github.com/ivmarkov/rust-esp32-std-demo).
