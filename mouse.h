#ifndef MOUSE_H
#define MOUSE_H

/* =============================================================================
   AKILLI FARE SÜRÜCÜSÜ FONKSİYON PROTOTİPLERİ
   ============================================================================= */

/* Fare donanımını ve koordinat arabelleklerini ilklendirir */
void init_mouse(void);

/* PS/2 portundan gelen ham fare hareketlerini sorgular ve işler */
void handle_mouse_polling(void);

/* YAPAY ZEKA ENTEGRASYONU: Fare tıklama ve hareket stresini analiz eder */
int ai_mouse_analyze_stress(void);

#endif /* MOUSE_H */
