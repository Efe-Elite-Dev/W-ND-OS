#!/bin/bash
set -e

echo "=== Wind OS Derleme Motoru Başlatıldı ==?"

# Eski kalıntıları temizle
rm -rf iso kernel.bin *.o

# 1. Assembly Dosyasını Derle (Executable stack uyarısını susturarak)
echo "[1/4] Assembly derleniyor..."
nasm -f elf32 boot.asm -o boot.o

# 2. Tüm C Kaynak Dosyalarını Derle
echo "[2/4] C kaynak kodları derleniyor..."
gcc -m32 -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c setup.c -o setup.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c gui.c -o gui.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c exe_subsystem.c -o exe_subsystem.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c ai_subsystem.c -o ai_subsystem.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c mouse.c -o mouse.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c keyboard.c -o keyboard.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c screen.c -o screen.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c idt.c -o idt.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c deb_subsystem.c -o deb_subsystem.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c uefi_subsystem.c -o uefi_subsystem.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c wind_subsystem.c -o wind_subsystem.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

# 3. Bağlama (Link) Aşaması - EKSİKSİZ TAM LİSTE
echo "[3/4] Tüm alt sistemler bağlanıyor (Linker)..."
# boot.o her zaman ilk sırada! executable-stack uyarısını kaldırmak için z noexecstack ekledik
ld -m elf_i386 -z noexecstack -T linker.ld -o kernel.bin \
    boot.o kernel.o setup.o gui.o exe_subsystem.o ai_subsystem.o \
    mouse.o keyboard.o screen.o idt.o deb_subsystem.o uefi_subsystem.o wind_subsystem.o

# Çekirdek kontrolü
if [ ! -f "kernel.bin" ]; then
    echo "HATA: kernel.bin üretilemedi!"
    exit 1
fi

# 4. ISO Yapısını Kurgula
echo "[4/4] ISO hiyerarşisi hazırlanıyor..."
mkdir -p iso/boot/grub
cp kernel.bin iso/boot/kernel.bin

cat << EOF > iso/boot/grub/grub.cfg
set timeout=0
set default=0

menuentry "Wind OS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o windos.iso iso
echo "=== BAŞARILI: windos.iso başarıyla mühürlendi! ==="
