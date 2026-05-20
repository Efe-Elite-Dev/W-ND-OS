#!/bin/bash
set -e
echo "[1] boot.asm derleniyor..."
nasm -f elf32 boot.asm -o boot.o

echo "[2] kernel.c derleniyor..."
gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector -O2 -Wall -c kernel.c -o kernel.o

echo "[3] Linkleniyor..."
ld -m elf_i386 -T linker.ld boot.o kernel.o -o kernel.bin

echo "[4] ISO yapısı oluşturuluyor..."
mkdir -p iso/boot/grub
cp kernel.bin iso/boot/kernel.bin
cp grub.cfg   iso/boot/grub/grub.cfg

echo "[5] ISO üretiliyor..."
grub-mkrescue -o windos.iso iso

echo "[6] Temizleniyor..."
rm -rf boot.o kernel.o kernel.bin iso

echo ""
echo "=== BASARILI: windos.iso hazir ==="
