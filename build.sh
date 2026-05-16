#!/bin/bash
set -e # Hata oluşursa hemen dur ki nerede patladığımızı bilelim!

echo "==> Wind OS Sıkıntıları Kıran Nihai Derleme Başlatıldı..."

rm -rf isodir windos_boot_layer
rm -f *.o kernel.bin windos.iso

mkdir -p windos_boot_layer/boot/grub

nasm -f elf32 boot.asm -o boot.o

echo "==> Çekirdek odaları ve AI modülleri derleniyor..."
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c kernel.c -o kernel.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c gui.c -o gui.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c exe_subsystem.c -o exe_subsystem.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c ai_subsystem.c -o ai_subsystem.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c mouse.c -o mouse.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c keyboard.c -o keyboard.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c wind_subsystem.c -o wind_subsystem.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c screen.c -o screen.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c idt.c -o idt.o
gcc -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -c deb_subsystem.c -o deb_subsystem.o

echo "==> Kurşun geçirmez linker bağlantısı yapılıyor..."
gcc -m32 -T linker.ld -nostdlib -no-pie -Wl,-z,noexecstack -o kernel.bin \
    boot.o kernel.o gui.o exe_subsystem.o ai_subsystem.o \
    mouse.o keyboard.o wind_subsystem.o screen.o idt.o deb_subsystem.o

cp kernel.bin windos_boot_layer/boot/kernel.bin

cat << 'EOF' > windos_boot_layer/boot/grub/grub.cfg
set timeout=0
set default=0
menuentry "Wind OS - Full AI Core" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o windos.iso windos_boot_layer
echo "==> BÜTÜN ARALIKLAR TAMAM! Gerçek ve çalışan ISO başarıyla mühürlendi."
