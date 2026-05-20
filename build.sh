name: Build WindOS Saf Kernel ISO

on:
  push:
    branches: [ "main", "master" ]
  pull_request:
    branches: [ "main", "master" ]
  workflow_dispatch: # GitHub Web arayüzünden manuel tetiklemek için buton ekler

jobs:
  build-iso:
    runs-on: ubuntu-latest

    steps:
    - name: Depoyu Klonla
      uses: actions/checkout@v4

    - name: Gerekli Bağımlılıkları Kur (NASM, GCC, GRUB, Xorriso)
      run: |
        sudo apt-get update
        sudo apt-get install -y nasm gcc-multilib grub-common grub-pc-bin xorriso mtools

    - name: Derleme Betiğini Çalıştır
      run: |
        chmod +x build.sh
        ./build.sh

    - name: Üretilen ISO Dosyasını Artifact Olarak Yükle
      uses: actions/upload-artifact@v4
      with:
        name: windos-saf-terminal-iso
        path: windos.iso
        if-no-files-found: error
