#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "fractal_land.hpp"
#include "pheronome.hpp"
#include "window.hpp"

class Renderer
{
public:
    Renderer(  const fractal_land& land,
               const pheronome& phen_food,
               const pheronome& phen_nest,
               const position_t& pos_nest,
               const position_t& pos_food,
               std::vector<int>& x,
               std::vector<int>& y,
               std::vector<uint8_t>& state,
               std::vector<std::size_t>& seeds,
               int nants );

    Renderer(const Renderer& ) = delete;
    ~Renderer();

    void display( Window& win, std::size_t const& compteur );
private:
    fractal_land const& m_ref_land;
    SDL_Texture* m_land{ nullptr }; 
    const pheronome& m_ref_phen_food;
    const pheronome& m_ref_phen_nest;
    const position_t& m_pos_nest;
    const position_t& m_pos_food;
    std::vector<int>& m_ref_x;
    std::vector<int>& m_ref_y;
    std::vector<uint8_t>& m_ref_state;
    std::vector<std::size_t>& m_ref_seeds;
    int m_nants;
    std::vector<std::size_t> m_curve;    
};
