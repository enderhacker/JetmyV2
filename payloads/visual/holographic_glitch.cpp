#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectHolographicGlitch(float time, float) {
    // Three different projected solids: cube, octahedron, and tetrahedron.
    constexpr float vertices[3][8][3] = {
        {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
        {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{0,0,0},{0,0,0}},
        {{1,1,1},{-1,-1,1},{-1,1,-1},{1,-1,-1},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}
    };
    constexpr int edges[3][12][2] = {
        {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}},
        {{0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{2,5},{3,4},{3,5}},
        {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}
    };
    float size = std::max(1.0f, std::min(g_ScreenWidth / 7.0f, g_ScreenHeight / 5.0f));
    for (int solid = 0; solid < 3; ++solid) {
        float a = time * (0.43f + solid * 0.12f), b = time * 0.31f + solid;
        float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
        float cx = g_ScreenWidth * (solid + 1) / 4.0f;
        float cy = g_ScreenHeight * (0.5f + 0.12f * sinf(time * 0.7f + solid));
        std::array<POINT, 8> projected{};
        for (int i = 0; i < (solid == 0 ? 8 : solid == 1 ? 6 : 4); ++i) {
            const auto& v = vertices[solid][i];
            float rx = v[0] * ca - v[2] * sa, rz = v[0] * sa + v[2] * ca;
            float ry = v[1] * cb - rz * sb, z = v[1] * sb + rz * cb;
            float perspective = 3.5f / (4.5f + z); // Strictly positive for every vertex.
            projected[i] = {LONG(cx + rx * size * perspective), LONG(cy + ry * size * perspective)};
        }
        bool glitch = sinf(time * 2.1f + solid * 4) > 0.97f;
        for (int e = 0; e < (solid == 2 ? 6 : 12); ++e) {
            const auto& p = projected[edges[solid][e][0]]; const auto& q = projected[edges[solid][e][1]];
            Line(p.x - 2, p.y, q.x - 2, q.y, 0x145C71);
            if (glitch) Line(p.x + 5, p.y + 2, q.x + 5, q.y + 2, 0xED69B3);
            Line(p.x, p.y, q.x, q.y, 0x8BF4EC);
        }
        // Ground projection / scan pedestal belongs only to the holograms.
        int ground = int(cy + size * 1.5f);
        for (int i = 0; i < 48; ++i) {
            float a0 = i * Tau / 48, a1 = (i + 1) * Tau / 48;
            Line(int(cx + cosf(a0) * size), int(ground + sinf(a0) * size * 0.2f),
                int(cx + cosf(a1) * size), int(ground + sinf(a1) * size * 0.2f), 0x306D7B);
        }
    }
}
}
