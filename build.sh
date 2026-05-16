#!/bin/bash
set -e

echo "==> Wind OS Özellikleri Korunarak Saf BIOS Modunda Paketleniyor..."

# 1. Eski kalıntıları temizle
rm -rf isodir
rm -f *.o kernel.bin windos.iso
mkdir -p isodir/boot/grub

# 2. Önyükleyici odasını derle
nasm -f elf32 boot.asm -o boot.o

# 3. TÜM ÖZELLİKLER VE AI ODALARI (DOKUNULMADI, %100 KORUNDU)
echo "==> AI Çekirdek odaları derleniyor..."
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

# 4. Hizalamalı Linker Bağlantısı
gcc -m32 -T linker.ld -nostdlib -no-pie -o kernel.bin \
    boot.o kernel.o gui.o exe_subsystem.o ai_subsystem.o \
    mouse.o keyboard.o wind_subsystem.o screen.o idt.o deb_subsystem.o

# 5. Dosyaları Yerleştir
cp kernel.bin isodir/boot/kernel.bin

# 6. GRUB Menü Konfigürasyonunu Doğrudan Enjekte Et
cat << 'EOF' > isodir/boot/grub/grub.cfg
set timeout=0
set default=0
menuentry "Wind OS - Full AI Core" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# 7. Multiboot Standart Doğrulaması
grub-file --is-x86-multiboot isodir/boot/kernel.bin

# 8. KRİTİK AYAR: ISO'yu sadece BIOS (i386-pc) formatında basmaya zorla
echo "==> Saf BIOS Boot Sektörü Paketleniyor..."
grub-mkrescue -d /usr/lib/grub/i386-pc -o windos.iso isodir

echo "==> İşlem Başarılı! Tüm AI özellikleri korundu ve BIOS ISO hazırlandı."
