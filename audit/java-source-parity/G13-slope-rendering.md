# G13 — Slope Tile Rendering (Tile 30–37) Mismatch

## Status: OPEN (new ticket, separate from BASE-001 parity)

## Deskripsi
Tile slope (ID 30–37: slope UP/RIGHT/DOWN/LEFT, regular + water) dirender salah:
- Tile 30–33 (slope 1): menampilkan **sprite bola** (lingkaran) bukannya segitiga diagonal
- Tile 34–37 (slope 2): menampilkan **kotak penuh** bukannya segitiga diagonal
- Variasi air (bit 0x40) juga salah
- Rotasi 90°/180°/270° mengikuti sprite dasar yang salah → terlihat "terbalik"

## Root Cause
`visual_build_sprites()` (visual_assets.c:629–652) memetakan tile slope ke koordinat atlas yang salah:

| Sprite | Digunakan untuk | Atlas (x,y) | Isi aktual |
|--------|----------------|-------------|------------|
| 55 (base 30) | 30–33 | (0,0) | **Bola** (lingkaran di baris 0) |
| 59 (water) | 30–33 | (0,0) | Bola + bg air |
| 63 (base 34) | 34–37 | (0,2) | **Kotak penuh** (baris 2) |
| 64–66 | 34–37 rotasi | rotasi 63 | Kotak penuh |

Atlas `objects_nm.png` (48×72 = 4×6 tile 12×12) **tidak punya tile slope bersih** di 12×12 utuh. Grafik segitiga hanya ada di baris 5 (pixel 66–68, 3 baris) sebagai bagian hoop quadrant.

## Dampak
- **Visual only** — collision/physics slope sudah benar (collision_query.c handle tile 30–37 via slope response u/v)
- Terlihat di banyak level (3, 5, 8, 11 dst) di lereng & ring

## Solusi Potensial
1. **Koordinat atlas benar** — butuh sprite sheet asli Nokia (PNG ini mungkin extract tidak lengkap)
2. **Programmatic generation** — render segitiga via renderer (refactor besar, perlu transform baru)
3. **Fallback texture** — copy dari hoop quadrant (baris 5) & crop ke 12×12 slope

## Priority
LOW (kosmetik). G1 (Continue K==1) diprioritaskan dulu.

---
Created: 2026-10-23
Related: BASE-001 audit (visual rendering not in scope)
