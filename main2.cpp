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

    // Variables de vida y Juego terminado
    int vidas = 3;
    int puntaje = 0;
    int limiteCajas = 20;   // Total de cajas que se pueden soltar (cuentan los aciertos y los fallos)
    int cajasUsadas = 0;
    bool juegoTerminado = false;
    const char* motivoFin = "";

    // Variables de la caída del bloque cuando se pierde una vida
    float velocidadCaidaFallo = 0.0f;
    float rotacionCaida = 0.0f;
    float direccionCaida = 1.0f;
    float tiempoFallo = 0.0f;

    Camera2D camara = { 0 };
    camara.target = (Vector2){ 0, 0 };
    camara.offset = (Vector2){ 0, 0 };
    camara.rotation = 0.0f;
    camara.zoom = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        float escala = dt * 60.0f; // Para que la fisica no dependa de los FPS

        if (tiempoFallo > 0) tiempoFallo -= dt;

        if (juegoTerminado)
        {
            // ==========================================
            // GAME OVER: solo esperamos la tecla R
            // ==========================================
            if (IsKeyPressed(KEY_R))
            {
                edificio.erase(edificio.begin() + 1, edificio.end()); // Dejamos solo la base
                camara.target.y = 0;
                posicionXBloque = 100;
                posicionYBloque = camara.target.y + 150;
                velocidadBloque = 350;
                estadoBloque = 0;
                anguloBalanceo = 0.0f;
                velocidadBalanceo = 0.0f;
                inestabilidadTotal = 0.0f;
                vidas = 3;
                puntaje = 0;
                cajasUsadas = 0;
                tiempoFallo = 0;
                juegoTerminado = false;
                motivoFin = "";
            }
        }
        else
        {
            //Usar la física del pendulo

            // La gravedad empuja la torre hacia el centro
            float fuerzaGravedad = -0.05f * anguloBalanceo;
            velocidadBalanceo += fuerzaGravedad * escala;

            // Multiplicador: A más pisos, más se nota la inestabilidad
            float factorAltura = edificio.size() * 0.02f;
            float limiteBalanceo = inestabilidadTotal * factorAltura;

            // Limitamos la velocidad del tambaleo según el error acumulado y la altura
            if (std::abs(velocidadBalanceo) > limiteBalanceo) {
                velocidadBalanceo = (velocidadBalanceo > 0) ? limiteBalanceo : -limiteBalanceo;
            }

            // Fricción: sin esto la torre oscilaría para siempre
            velocidadBalanceo *= std::pow(0.995f, escala);

            anguloBalanceo += velocidadBalanceo * escala;

            // Si la torre se inclina demasiado, se cae y termina el juego (ajusta el 5.0f a tu gusto)
            if (std::abs(anguloBalanceo) > 5.0f) {
                juegoTerminado = true;
                motivoFin = "La torre se inclino demasiado";
            }

            // 2.lógica del juego
            if (estadoBloque == 0)
            {
                posicionXBloque += velocidadBloque * GetFrameTime();

                if (posicionXBloque > anchoPantalla - anchoActual - 100) velocidadBloque = -350;
                if (posicionXBloque < 100) velocidadBloque = 350;

                if (IsKeyPressed(KEY_SPACE)) {
                    estadoBloque = 1;
                    cajasUsadas++;
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

                    bool perfecto = false;

                    // Perfect hit: Lo alineamos al centro y no suma inestabilidad
                    if (std::abs(diferencia) < 10.0f) {
                        posicionXBloque = ultimoPiso.x + (ultimoPiso.ancho/2.0f) - (anchoActual/2.0f);
                        diferencia = 0;
                        perfecto = true;
                    }

                    // FALLO: Cayó fuera del piso. La torre NO se toca, se pierde una vida
                    if (std::abs(diferencia) > (ultimoPiso.ancho / 2.0f))
                    {
                        vidas--;

                        // Preparamos la animación de caída al vacío
                        direccionCaida = (diferencia >= 0) ? 1.0f : -1.0f;
                        velocidadCaidaFallo = -250.0f; // Un pequeño rebote hacia arriba antes de caer
                        rotacionCaida = 0.0f;
                        tiempoFallo = 1.2f;            // Duración del destello rojo y el aviso en pantalla
                        estadoBloque = 2;
                    }
                    else // CONDICIÓN DE ÉXITO: Aterrizó pero genera inestabilidad
                    {
                        Piso nuevoPiso = { posicionXBloque, posicionYBloque, anchoActual, 40, DARKGRAY };
                        edificio.push_back(nuevoPiso);

                        // Puntaje: 10 por piso, +50 si fue perfecto
                        puntaje += 10;
                        if (perfecto) puntaje += 50;

                        // Aumentamos la inestabilidad base
                        inestabilidadTotal += std::abs(diferencia) * 0.003f;

                        // Empujón a la torre por la caída del bloque
                        velocidadBalanceo += diferencia * 0.001f;

                        // La cámara sube un piso y la grúa reaparece siempre a la misma altura en pantalla
                        camara.target.y -= 40;
                        posicionYBloque = camara.target.y + 150;
                        estadoBloque = 0;

                        // Si era la última caja, termina la partida
                        if (cajasUsadas >= limiteCajas) {
                            juegoTerminado = true;
                            motivoFin = "Se acabaron las cajas";
                        }
                    }
                }
            }
            else if (estadoBloque == 2)
            {
                // ANIMACIÓN: el bloque fallido rueda por el borde y cae al vacío
                velocidadCaidaFallo += 900 * dt;
                posicionYBloque += velocidadCaidaFallo * dt;
                posicionXBloque += direccionCaida * 120 * dt;
                rotacionCaida += direccionCaida * 280 * dt;

                // Cuando sale de la pantalla, la grúa reaparece
                if (posicionYBloque > camara.target.y + altoPantalla + 100)
                {
                    posicionYBloque = camara.target.y + 150;
                    rotacionCaida = 0.0f;
                    estadoBloque = 0;

                    if (vidas <= 0) {
                        juegoTerminado = true;
                        motivoFin = "Te quedaste sin vidas";
                    }
                    else if (cajasUsadas >= limiteCajas) {
                        juegoTerminado = true;
                        motivoFin = "Se acabaron las cajas";
                    }
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
            }

            // Dibujar grúa y bloque colgante
            if (estadoBloque == 0) {
                DrawLine(anchoPantalla / 2, camara.target.y, (int)posicionXBloque + anchoActual/2, (int)posicionYBloque, BLACK);
            }

            if (estadoBloque == 2)
            {
                // Bloque fallido: rojo y girando mientras cae
                Rectangle rectCaida = { posicionXBloque + anchoActual / 2.0f, posicionYBloque + 20.0f, anchoActual, 40.0f };
                Vector2 origenCaida = { anchoActual / 2.0f, 20.0f };
                DrawRectanglePro(rectCaida, origenCaida, rotacionCaida, RED);
            }
            else
            {
                DrawRectangle((int)posicionXBloque, (int)posicionYBloque, (int)anchoActual, 40, GRAY);
                DrawRectangleLines((int)posicionXBloque, (int)posicionYBloque, (int)anchoActual, 40, BLACK);
            }

        EndMode2D();

        // Destello rojo y aviso cuando se pierde una vida
        if (tiempoFallo > 0)
        {
            DrawRectangle(0, 0, anchoPantalla, altoPantalla, Fade(RED, 0.25f * (tiempoFallo / 1.2f)));

            const char* aviso = "FALLASTE!  -1 VIDA";
            DrawText(aviso, (anchoPantalla - MeasureText(aviso, 50)) / 2, 200, 50, MAROON);
        }

        DrawText("RECONSTRUYENDO YACHAY", 20, 20, 30, BLACK);
        DrawText(TextFormat("PISOS: %d", (int)edificio.size() - 1), 20, 60, 20, DARKBLUE);
        DrawText(TextFormat("VIDAS: %d", vidas), 20, 90, 20, RED);
        DrawText(TextFormat("PUNTAJE: %d", puntaje), 20, 120, 20, DARKGREEN);
        DrawText(TextFormat("CAJAS: %d / %d", cajasUsadas, limiteCajas), 20, 180, 20, DARKPURPLE);
        // Util para ajustar el limite de inclinacion; se puede borrar al final
        DrawText(TextFormat("INCLINACION: %.2f", std::abs(anguloBalanceo)), 20, 150, 20, DARKGRAY);

        if (juegoTerminado)
        {
            DrawRectangle(0, 0, anchoPantalla, altoPantalla, Fade(BLACK, 0.6f));

            const char* titulo = "GAME OVER";
            DrawText(titulo, (anchoPantalla - MeasureText(titulo, 60)) / 2, 280, 60, WHITE);
            DrawText(motivoFin, (anchoPantalla - MeasureText(motivoFin, 24)) / 2, 360, 24, WHITE);

            const char* resultado = TextFormat("Pisos: %d   Puntaje: %d", (int)edificio.size() - 1, puntaje);
            DrawText(resultado, (anchoPantalla - MeasureText(resultado, 24)) / 2, 400, 24, WHITE);

            const char* reinicio = "Presiona R para reiniciar";
            DrawText(reinicio, (anchoPantalla - MeasureText(reinicio, 24)) / 2, 450, 24, YELLOW);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
