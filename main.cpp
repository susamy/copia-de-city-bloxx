#include "raylib.h"
#include <vector>
#include <cmath>

struct Piso {
    float x;
    float y;
    float ancho;
    float alto;
    Color color;
};

int main(void)
{
    const int anchoPantalla = 1200;
    const int altoPantalla = 800;
    InitWindow(anchoPantalla, altoPantalla, "Reconstruyendo Yachay");

    std::vector<Piso> edificio; 
    Piso base = { 450, 700, 300, 40, BROWN };
    edificio.push_back(base);

    float anchoActual = 120;
    float posicionXBloque = 100;
    float posicionYBloque = 150;
    float velocidadBloque = 350;
    int estadoBloque = 0; 

    // Variables de física
    float anguloBalanceo = 0.0f;     
    float velocidadBalanceo = 0.0f;  
    float inestabilidadTotal = 0.0f; 

    Camera2D camara = { 0 };
    camara.target = (Vector2){ 0, 0 };
    camara.offset = (Vector2){ 0, 0 };
    camara.rotation = 0.0f;
    camara.zoom = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // ==========================================
        // 1. FÍSICA DEL PÉNDULO (Dependiente de la altura)
        // ==========================================
        
        // La gravedad empuja la torre hacia el centro
        float fuerzaGravedad = -0.05f * anguloBalanceo;
        velocidadBalanceo += fuerzaGravedad;
        
        // Multiplicador: A más pisos, más se nota la inestabilidad
        float factorAltura = edificio.size() * 0.02f; 
        float limiteBalanceo = inestabilidadTotal * factorAltura;
        
        // Limitamos la velocidad del tambaleo según el error acumulado y la altura
        if (std::abs(velocidadBalanceo) > limiteBalanceo) {
            velocidadBalanceo = (velocidadBalanceo > 0) ? limiteBalanceo : -limiteBalanceo;
        }

        anguloBalanceo += velocidadBalanceo;

        // ==========================================
        // 2. LÓGICA DEL JUEGO
        // ==========================================
        if (estadoBloque == 0) 
        {
            posicionXBloque += velocidadBloque * GetFrameTime();

            if (posicionXBloque > anchoPantalla - anchoActual - 100) velocidadBloque = -350;
            if (posicionXBloque < 100) velocidadBloque = 350;

            if (IsKeyPressed(KEY_SPACE)) {
                estadoBloque = 1;
            }
        } 
        else if (estadoBloque == 1) 
        {
            posicionYBloque += 600 * GetFrameTime();
            Piso ultimoPiso = edificio.back();
            
            if (posicionYBloque >= ultimoPiso.y - 40) 
            {
                posicionYBloque = ultimoPiso.y - 40; 
                
                float centroAnterior = ultimoPiso.x + (ultimoPiso.ancho / 2.0f);
                float centroActual = posicionXBloque + (anchoActual / 2.0f);
                float diferencia = centroActual - centroAnterior;
                
                // Perfect hit: Lo alineamos al centro y no suma inestabilidad
                if (std::abs(diferencia) < 10.0f) {
                    posicionXBloque = ultimoPiso.x + (ultimoPiso.ancho/2.0f) - (anchoActual/2.0f);
                    diferencia = 0; 
                }
                
                // CONDICIÓN DE DERRUMBE: Cayó demasiado al borde y choca contra los pisos
                if (std::abs(diferencia) > (ultimoPiso.ancho / 2.0f)) 
                {
                    // Tumbamos hasta 2 pisos (dependiendo de cuántos haya)
                    int pisosATumbar = 2; 
                    for (int j = 0; j < pisosATumbar; j++) {
                        if (edificio.size() > 1) { // Aseguramos no borrar la base marrón
                            edificio.pop_back();
                            camara.target.y += 40; // Bajamos la cámara
                        }
                    }

                    // Reducimos un poco la inestabilidad porque cayeron los bloques chuecos de arriba
                    inestabilidadTotal *= 0.7f; 
                    velocidadBalanceo = 0; // Pausamos el tambaleo un segundo por el impacto

                    // Reiniciamos la grúa a la nueva altura
                    posicionYBloque = camara.target.y + 150; 
                    estadoBloque = 0;
                } 
                else // CONDICIÓN DE ÉXITO: Aterrizó pero genera inestabilidad
                {
                    Piso nuevoPiso = { posicionXBloque, posicionYBloque, anchoActual, 40, DARKGRAY };
                    edificio.push_back(nuevoPiso);

                    // Aumentamos la inestabilidad base
                    inestabilidadTotal += std::abs(diferencia) * 0.003f;
                    
                    // Empujón a la torre por la caída del bloque
                    velocidadBalanceo += diferencia * 0.001f;

                    posicionYBloque -= 250;
                    estadoBloque = 0;
                    camara.target.y -= 40;
                }
            }
        }

        // ==========================================
        // 3. DIBUJO EN PANTALLA
        // ==========================================
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode2D(camara);

            // Dibujar la base estática
            DrawRectangle((int)edificio[0].x, (int)edificio[0].y, (int)edificio[0].ancho, (int)edificio[0].alto, edificio[0].color);
            DrawRectangleLines((int)edificio[0].x, (int)edificio[0].y, (int)edificio[0].ancho, (int)edificio[0].alto, BLACK);

            // Dibujar los pisos inclinándolos desde la base central
            for (int i = 1; i < edificio.size(); i++) {
                
                Rectangle rect = { 600.0f, 700.0f, edificio[i].ancho, edificio[i].alto };
                Vector2 origin = { 600.0f - edificio[i].x, 700.0f - edificio[i].y };
                
                DrawRectanglePro(rect, origin, anguloBalanceo, edificio[i].color);
                DrawRectanglePro(rect, origin, anguloBalanceo, Fade(BLACK, 0.5f)); 
            }

            // Dibujar grúa y bloque colgante
            if (estadoBloque == 0) {
                DrawLine(anchoPantalla / 2, camara.target.y, (int)posicionXBloque + anchoActual/2, (int)posicionYBloque, BLACK);
            }

            DrawRectangle((int)posicionXBloque, (int)posicionYBloque, (int)anchoActual, 40, GRAY);
            DrawRectangleLines((int)posicionXBloque, (int)posicionYBloque, (int)anchoActual, 40, BLACK);

        EndMode2D();

        DrawText("RECONSTRUYENDO YACHAY", 20, 20, 30, BLACK);
        DrawText(TextFormat("PISOS: %d", (int)edificio.size() - 1), 20, 60, 20, DARKBLUE);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}