#include <limits>
#include <algorithm>
#include "renderer.hpp"

Renderer::Renderer(const fractal_land& land,
                   const pheronome& phen_food,
                   const pheronome& phen_nest,
                   const position_t& pos_nest,
                   const position_t& pos_food,
                   std::vector<int>& x,
                   std::vector<int>& y,
                   std::vector<uint8_t>& state,
                   std::vector<std::size_t>& seeds,
                   int nants)
    : m_ref_land(land),
      m_land(nullptr),
      m_ref_phen_food(phen_food),
      m_ref_phen_nest(phen_nest),
      m_pos_nest(pos_nest),
      m_pos_food(pos_food),
      m_ref_x(x),
      m_ref_y(y),
      m_ref_state(state),
      m_ref_seeds(seeds),
      m_nants(nants)
{
    // Note: La texture sera créée lors du premier display()
}

Renderer::~Renderer() {
    if (m_land != nullptr)
        SDL_DestroyTexture(m_land);
}

void Renderer::display(Window& win, std::size_t const& compteur)
{
    SDL_Renderer* renderer = SDL_GetRenderer(win.get());

    // Créer la texture du paysage si elle n'existe pas encore
    if (m_land == nullptr) {
        SDL_Surface* temp_surface = SDL_CreateRGBSurface(
            0,
            static_cast<int>(m_ref_land.dimensions()),
            static_cast<int>(m_ref_land.dimensions()),
            32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
        );

        double min_height = std::numeric_limits<double>::max();
        double max_height = std::numeric_limits<double>::lowest();

        for (fractal_land::dim_t i = 0; i < m_ref_land.dimensions(); ++i)
            for (fractal_land::dim_t j = 0; j < m_ref_land.dimensions(); ++j) {
                min_height = std::min(min_height, m_ref_land(i, j));
                max_height = std::max(max_height, m_ref_land(i, j));
            }

        for (fractal_land::dim_t i = 0; i < m_ref_land.dimensions(); ++i)
            for (fractal_land::dim_t j = 0; j < m_ref_land.dimensions(); ++j) {
                double c = 255.0 * (m_ref_land(i, j) - min_height) / (max_height - min_height);
                Uint32* pixel = (Uint32*)((Uint8*)temp_surface->pixels + j * temp_surface->pitch + i * sizeof(Uint32));
                *pixel = SDL_MapRGBA(temp_surface->format,
                                     static_cast<Uint8>(c),
                                     static_cast<Uint8>(c),
                                     static_cast<Uint8>(c),
                                     255);
            }

        m_land = SDL_CreateTextureFromSurface(renderer, temp_surface);
        SDL_FreeSurface(temp_surface);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Rect dest_rect1{0, 0,
                        static_cast<int>(m_ref_land.dimensions()),
                        static_cast<int>(m_ref_land.dimensions())};
    SDL_RenderCopy(renderer, m_land, nullptr, &dest_rect1);

    SDL_Rect dest_rect2{static_cast<int>(m_ref_land.dimensions()) + 10, 0,
                        static_cast<int>(m_ref_land.dimensions()),
                        static_cast<int>(m_ref_land.dimensions())};
    SDL_RenderCopy(renderer, m_land, nullptr, &dest_rect2);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Affichage des fourmis (haut gauche)
    for (int i = 0; i < m_nants; ++i) {
        win.set_pen(0, 255, 255);
        win.pset(m_ref_x[i], m_ref_y[i]);
    }

    // Affichage des phéromones (haut droite)
    for (fractal_land::dim_t i = 0; i < m_ref_land.dimensions(); ++i)
        for (fractal_land::dim_t j = 0; j < m_ref_land.dimensions(); ++j) {
            double r = std::min(1.0, m_ref_phen_food(i, j));
            double g = std::min(1.0, m_ref_phen_nest(i, j));

            if (r > 0.01 || g > 0.01) {
                win.set_pen(static_cast<Uint8>(r * 255.0),
                            static_cast<Uint8>(g * 255.0),
                            0);
                win.pset(static_cast<int>(i + m_ref_land.dimensions() + 10),
                         static_cast<int>(j));
            }
        }

    // Courbe d'enfouragement
    m_curve.push_back(compteur);
    if (m_curve.size() > 1) {
        int sz_win = win.size().first;
        int ydec   = win.size().second - 1;

        double max_curve_val = *std::max_element(m_curve.begin(), m_curve.end());
        double h_max_val = 256.0 / std::max(max_curve_val, 1.0);
        double step = double(sz_win) / double(m_curve.size());

        SDL_SetRenderDrawColor(renderer, 255, 255, 127, 255);
        for (std::size_t i = 0; i + 1 < m_curve.size(); ++i) {
            int x1 = static_cast<int>(i * step);
            int y1 = static_cast<int>(ydec - m_curve[i] * h_max_val);
            int x2 = static_cast<int>((i + 1) * step);
            int y2 = static_cast<int>(ydec - m_curve[i + 1] * h_max_val);
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
    }

    SDL_RenderPresent(renderer);
}
