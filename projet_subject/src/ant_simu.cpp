#include <vector>
#include <iostream>
#include <random>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
# include "renderer.hpp"
# include "window.hpp"
# include "rand_generator.hpp"

void advance_time( const fractal_land& land,
                   pheronome& phen_food,
                   pheronome& phen_nest,
                   const position_t& pos_nest,
                   const position_t& pos_food,
                   std::vector<int>& x,
                   std::vector<int>& y,
                   std::vector<uint8_t>& state,
                   std::vector<std::size_t>& seeds,
                   std::size_t& cpteur,
                   double eps )
{
    std::vector<std::size_t> touched_food;
    std::vector<std::size_t> touched_nest;
    touched_food.reserve(x.size());
    touched_nest.reserve(x.size());

    for ( size_t i = 0; i < x.size(); ++i )
        advance_ant( phen_food, phen_nest, land, pos_food, pos_nest,
                     cpteur, seeds[i], x[i], y[i], state[i], eps,
                     touched_food, touched_nest );

    phen_food.apply_marks_from_touched(touched_food);
    phen_nest.apply_marks_from_touched(touched_nest);
    phen_food.do_evaporation();
    phen_nest.do_evaporation();
    phen_food.update();
    phen_nest.update();
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    std::size_t seed0 = 2026;          // Graine globale (reproductible)
    const int nb_ants = 5000;
    const double eps  = 0.8;
    const double alpha = 0.7;
    const double beta  = 0.999;

    position_t pos_nest{256,256};
    position_t pos_food{500,500};

    fractal_land land(8, 2, 1., 1024);

    double max_val = 0.0;
    double min_val = 0.0;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i)
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }

    double delta = max_val - min_val;

    /* Normalisation : valeurs dans [0,1] */
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i)
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j)
            land(i,j) = (land(i,j) - min_val) / delta;

    // Données par fourmi (si tu ne construis plus vector<ant> ici)
    std::vector<int> x(nb_ants);
    std::vector<int> y(nb_ants);
    std::vector<uint8_t> state(nb_ants);
    std::vector<std::size_t> seeds(nb_ants);

    auto gen_ant_pos = [&land, &seed0]() {
        return rand_int32(0, static_cast<int32_t>(land.dimensions() - 1), seed0);
    };

    for (size_t i = 0; i < static_cast<size_t>(nb_ants); ++i) {
        x[i] = gen_ant_pos();
        y[i] = gen_ant_pos();
        state[i] = 0;
        seeds[i] = static_cast<uint32_t>(seed0 + i);  // Une graine par fourmi (simple)
    }

    const unsigned long dim = static_cast<unsigned long>(land.dimensions());

    // Deux champs séparés : un vers la nourriture, un vers le nid
    pheronome phen_food(dim, pos_food, alpha, beta);
    pheronome phen_nest(dim, pos_nest, alpha, beta);

    Window win("Ant Simulation", 2*land.dimensions()+10, land.dimensions()+266);
    
    Renderer renderer( land, phen_food, phen_nest, pos_nest, pos_food,
                       x, y, state, seeds, nb_ants );
    // Compteur de la quantité de nourriture apportée au nid par les fourmis
    size_t food_quantity = 0;
    SDL_Event event;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;
    while (cont_loop) {
        ++it;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                cont_loop = false;
        }
        advance_time( land, phen_food, phen_nest, pos_nest, pos_food,
                      x, y, state, seeds, food_quantity, eps );
        renderer.display( win, food_quantity );
        win.blit();
        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << std::endl;
            not_food_in_nest = false;
        }
        //SDL_Delay(10);
    }
    SDL_Quit();
    return 0;
}
