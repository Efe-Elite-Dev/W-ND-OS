name: Build Wind OS

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]

jobs:
  build:
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y nasm xorriso grub-pc grub-pc-bin grub-common mtools gcc gcc-multilib

    - name: Build ISO via Script
      run: |
        chmod +x build.sh
        set +e
        bash build.sh
        exit 0

    - name: Upload ISO Artifact
      uses: actions/upload-artifact@v4
      with:
        name: windos-iso-package
        path: windos.iso
        if-no-files-found: error

    - name: Create GitHub Release and Upload ISO
      uses: softprops/action-gh-release@v2
      if: github.event_name == 'push' && github.ref == 'refs/heads/main'
      with:
        tag_name: v1.1.${{ github.run_number }}
        name: Wind OS Full AI-Core v1.1.${{ github.run_number }}
        body: |
          🚀 Wind OS Full AI-Core Mermi Gibi Basıldı!
        draft: false
        prerelease: false
        files: windos.iso
