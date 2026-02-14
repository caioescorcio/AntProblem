#include "population.hpp"
#include <iostream>
#include "rand_generator.hpp"

void Population::advance(int index, pheronome& phen, const fractal_land& land, const position_t& pos_food, const position_t& pos_nest,
                   std::size_t& cpteur_food) {

    auto ant_choice = [this, index]() mutable { return rand_double( 0., 1., this->seeds[index] ); };   // choise based on most likely pheromone route
    auto dir_choice = [this, index]() mutable { return rand_int32( 1, 4, this->seeds[index] ); };      // random choice in case of no good pheromone route
    const int dimx = static_cast<int>(land.dimensions());
    const int dimy = static_cast<int>(land.height());
    auto in_storage = [dimx, dimy](const position_t& p) {
        return (p.x >= 0) && (p.x < dimx) && (p.y >= 0) && (p.y < dimy);
    };


    auto in_stencil = [dimx, dimy](const position_t& p) {
        return (p.x > 0) 
        && (p.x + 1 < dimx) && (p.y > 0) && (p.y + 1 < dimy);
    };


    double consumed_time = 0.;
    while ( consumed_time < 1. ) {
        // Si la fourmi est chargée, elle suit les phéromones de deuxième type, sinon ceux du premier.
        int        ind_pher    = ( is_loaded(index) ? 1 : 0 );
        double     choix       = ant_choice( );
        position_t old_pos_ant = get_position(index);
        // Keep position inside local storage.
        if (!in_storage(old_pos_ant)) {
            if (old_pos_ant.x < 0) old_pos_ant.x = 0;
            if (old_pos_ant.x >= dimx) old_pos_ant.x = dimx - 1;
            if (old_pos_ant.y < 0) old_pos_ant.y = 0;
            if (old_pos_ant.y >= dimy) old_pos_ant.y = dimy - 1;
            positions[index] = old_pos_ant;
        }
        // Migration phase will transfer it to neighbor rank.
        if (!in_stencil(old_pos_ant)) {
            break;
        }
        position_t new_pos_ant = old_pos_ant;
        double max_phen    = std::max( {phen( new_pos_ant.x - 1, new_pos_ant.y )[ind_pher],
                                     phen( new_pos_ant.x + 1, new_pos_ant.y )[ind_pher],
                                     phen( new_pos_ant.x, new_pos_ant.y - 1 )[ind_pher],
                                     phen( new_pos_ant.x, new_pos_ant.y + 1 )[ind_pher]} );
        if ((choix > m_eps) || (max_phen <= 0.0)) {
            bool moved = false;
            for (int t = 0; t < 8 && !moved; ++t) {
                position_t cand = old_pos_ant;
                int d = dir_choice();
                if (d == 1) cand.x -= 1;
                if (d == 2) cand.y -= 1;
                if (d == 3) cand.x += 1;
                if (d == 4) cand.y += 1;

                if (in_storage(cand) && phen[cand][ind_pher] != -1) {
                    new_pos_ant = cand;
                    moved = true;
                }
            }
        } else {
            if ( phen( new_pos_ant.x - 1, new_pos_ant.y )[ind_pher] == max_phen )
                new_pos_ant.x -= 1;
            else if ( phen( new_pos_ant.x + 1, new_pos_ant.y )[ind_pher] == max_phen )
                new_pos_ant.x += 1;
            else if ( phen( new_pos_ant.x, new_pos_ant.y - 1 )[ind_pher] == max_phen )
                new_pos_ant.y -= 1;
            else  // if (phen(new_pos_ant.first,new_pos_ant.second+1)[ind_pher] == max_phen)
                new_pos_ant.y += 1;

            if (!in_storage(new_pos_ant) || phen[new_pos_ant][ind_pher] == -1) {
                new_pos_ant = old_pos_ant;
            }
        }
        consumed_time += land( new_pos_ant.x, new_pos_ant.y);
        phen.mark_pheronome( new_pos_ant );
        positions[index] = new_pos_ant;
        if ( get_position(index) == pos_nest ) {
            if ( is_loaded(index) ) {
                cpteur_food += 1;
            }
            unset_loaded(index);
        }
        if ( get_position(index) == pos_food ) {
            set_loaded(index);
        }
        if (!in_stencil(new_pos_ant)) {
            break;
        }
    }
}
