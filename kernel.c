// 1. Kurulum Ekranı (OOBE)
void draw_oobe_setup() {
    fill_screen(0xFF0078D4); // Mavi zemin
    draw_str(300, 300, "WIND OS KURULUMU", 0xFFFFFFFF, 3);
    draw_str(300, 350, "ENTER'a basarak devam et.", 0xFFFFFFFF, 1);
}

// 2. Masaüstü ve EXE Çalıştırma
void draw_desktop() {
    fill_screen(0xFF1A1A1A); // Koyu gri masaüstü
    draw_str(20, 20, "Masaustu - Wind OS v1.5", 0xFF00E5FF, 1);
    
    // Basit bir ikon/buton
    draw_rect(50, 50, 100, 100, 0xFF444444);
    draw_str(60, 90, "EXE 1", 0xFFFFFFFF, 1);
}

void execute_app(int app_id) {
    // Burada process yönetimine geçiş yapılacak
    fill_screen(0xFF000000); 
    draw_str(400, 300, "Uygulama Calisiyor...", 0xFF00FF00, 2);
}
