import customtkinter as ctk
from sky_setup_ui import SkyCoreSetup

class SkyKernelCore(ctk.CTk):
    def __init__(self):
        super().__init__()
        
        # Ana İşletim Sistemi Ekran Parametreleri
        self.title("Sky Core OS v1.5 [vortex-kernel] - Active")
        self.geometry("1024x768")
        self.resizable(False, False)
        
        # Ana Arka Plan ve Düzen Paneli
        self.kernel_frame = ctk.CTkFrame(self, fg_color="#0f0a1c")
        self.kernel_frame.pack(fill="both", expand=True)
        
        self.load_main_os_interface()

    def load_main_os_interface(self):
        # Ana kernel arayüz tasarımı, fırtına çizgileri ve widget'lar buraya yüklenir
        status_lbl = ctk.CTkLabel(
            self.kernel_frame, 
            text="🌪️ SKY CORE OS v1.5 ACTIVATED\n[vortex-kernel] Başarıyla Yüklendi ve Çalışıyor.", 
            font=("Arial", 20, "bold"), 
            text_color="#00d2d3"
        )
        status_lbl.pack(pady=100)
        
        # Buraya kendi mevcut kernel'inin diğer bütün alt fonksiyonlarını, terminal simülatörlerini ekleyebilirsin
        terminal_mock = ctk.CTkTextbox(self.kernel_frame, width=700, height=300, fg_color="black", text_color="green", font=("Consolas", 12))
        terminal_mock.pack(pady=20)
        terminal_mock.insert("0.0", "sky_core@vortex-kernel:~$ core_init --status SUCCESS\nsky_core@vortex-kernel:~$ Tüm sistem bileşenleri kararlı. VirtualBox entegrasyonu hazır.\nsky_core@vortex-kernel:~$ ")

# --- İKİ SİSTEMİ BİRBİRİNE BAĞLAYAN ANA TETİKLEYİCİ MİMARİ ---
def boot_sequence():
    print("[BOOT] Kurulum bitti, ana Sky Core OS Kernel devreye alınıyor...")
    # Kurulum sihirbazı bittiği an bu fonksiyon çalışır ve ana işletim sistemini ayağa kaldırır
    main_os = SkyKernelCore()
    main_os.mainloop()

if __name__ == "__main__":
    print("[BOOT] Sky Core OS Kurulum Sihirbazı Başlatılıyor...")
    # İlk önce kurulum sihirbazını başlatıyoruz ve bitiş fonksiyonuna boot_sequence'ı bağlıyoruz
    setup_wizard = SkyCoreSetup(on_complete_callback=boot_sequence)
    setup_wizard.mainloop()
