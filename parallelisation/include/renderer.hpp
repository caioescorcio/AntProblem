#pragma once
#include "fractal_land.hpp"
#include "pheronome.hpp"
#include "window.hpp"
#include "population.hpp"

class Renderer
{
public:
    Renderer(  const fractal_land& land, const pheronome& phen, 
               const position_t& pos_nest, const position_t& pos_food,
               const Population& pop );

    Renderer(const Renderer& ) = delete;
    ~Renderer();

    void display( Window& win, std::size_t const& compteur );
private:
    fractal_land const& m_ref_land;
    SDL_Texture* m_land{ nullptr }; 
    const pheronome& m_ref_phen;
    const position_t& m_pos_nest;
    const position_t& m_pos_food;
    const Population& m_ref_pop;
    std::vector<std::size_t> m_curve;    
};