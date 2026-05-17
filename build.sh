#!/bin/bash
set -e

echo "=== Wind OS Derleme Motoru Başlatıldı ==="

# Eski kalıntıları temizle
rm -rf iso kernel.bin *.o

echo "[1/4] Assembly derleniyor..."
nasm -f elf32 boot.asm -o boot.o

echo "[2/4] C kaynak kodları dinamik olarak derleniyor..."
# Temel sistem dosyaları
gcc -m32 -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c setup.c -o setup.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

# İsteğe bağlı alt sistemlerin varlık kontrolü (Hata önleyici katman)
OPTIONAL_OBJS=""
for file in gui exe_subsystem ai_subsystem mouse keyboard screen idt deb_subsystem uefi_subsystem wind_subsystem; do
    if [ -f "${file}.c" ]; then
        gcc -m32 -c "${file}.c" -o "${file}.o" -std=gnu99 -ffreestanding -O2 -Wall -Wextra
        OPTIONAL_OBJS="$OPTIONAL_OBJS ${file}.o"
    else
        echo "--> Bilgi: ${file}.c bulunamadi, bu modül bypass ediliyor."
    fi
done

echo "[3/4] Tüm 3D alt sistemler bağlanıyor (Linker)..."
# boot.o her zaman ilk sırada kalmak zorunda!
ld -m elf_i386 -z noexecstack -T linker.ld -o kernel.bin boot.o kernel.o setup.o $OPTIONAL_OBJS

# Çekirdek dosyasının üretildiğini doğrula
if [ ! -f "kernel.bin" ]; then
    echo "HATA: kernel.bin linker tarafından üretilemedi!"
    exit 1
fi

echo "[4/4] ISO hiyerarşisi hazırlanıyor..."
mkdir -p iso/boot/grub
cp kernel.bin iso/boot/kernel.bin

# grub.cfg dosyasını oluştur
cat << EOF > iso/boot/grub/grub.cfg
set timeout=0
set default=0

menuentry "Wind OS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# ISO Dosyasını oluştur ve mühürle
grub-mkrescue -o windos.iso iso

echo "=== BAŞARILI: windos.iso başarıyla mühürlendi! ==="
