#include "population.hpp"
#include <iostream>
#include "rand_generator.hpp"

void Population::advance(int index, pheronome& phen, const fractal_land& land, const position_t& pos_food, const position_t& pos_nest,
                   std::size_t& cpteur_food) {

    auto ant_choice = [this, index]() mutable { return rand_double( 0., 1., this->seeds[index] ); };   // choise based on most likely pheromone route
    auto dir_choice = [this, index]() mutable { return rand_int32( 1, 4, this->seeds[index] ); };      // random choice in case of no good pheromone route
    const int x_min = 1;
    const int y_min = 1;
    const int x_max = static_cast<int>(land.dimensions()) - 2;
    const int y_max = static_cast<int>(land.height()) - 2;
    auto is_inside = [x_min, x_max, y_min, y_max](const position_t& p) {
        return (p.x >= x_min) && (p.x <= x_max) &&
               (p.y >= y_min) && (p.y <= y_max);
    };

    double consumed_time = 0.;
    while ( consumed_time < 1. ) {
        // Si la fourmi est chargée, elle suit les phéromones de deuxième type, sinon ceux du premier.
        int        ind_pher    = ( is_loaded(index) ? 1 : 0 );
        double     choix       = ant_choice( );
        position_t old_pos_ant = get_position(index);
        if (!is_inside(old_pos_ant)) {
            if (old_pos_ant.x < x_min) old_pos_ant.x = x_min;
            if (old_pos_ant.x > x_max) old_pos_ant.x = x_max;
            if (old_pos_ant.y < y_min) old_pos_ant.y = y_min;
            if (old_pos_ant.y > y_max) old_pos_ant.y = y_max;
            positions[index] = old_pos_ant;
        }
        position_t new_pos_ant = old_pos_ant;
        double max_phen    = std::max( {phen( new_pos_ant.x - 1, new_pos_ant.y )[ind_pher],
                                     phen( new_pos_ant.x + 1, new_pos_ant.y )[ind_pher],
                                     phen( new_pos_ant.x, new_pos_ant.y - 1 )[ind_pher],
                                     phen( new_pos_ant.x, new_pos_ant.y + 1 )[ind_pher]} );
        if ( ( choix > m_eps ) || ( max_phen <= 0. ) ) {
            do {
                new_pos_ant = old_pos_ant;
                int d = dir_choice();
                if ( d==1 ) new_pos_ant.x  -= 1;
                if ( d==2 ) new_pos_ant.y -= 1;
                if ( d==3 ) new_pos_ant.x  += 1;
                if ( d==4 ) new_pos_ant.y += 1;

            } while ( !is_inside(new_pos_ant) || phen[new_pos_ant][ind_pher] == -1 );
        } else {
            if ( phen( new_pos_ant.x - 1, new_pos_ant.y )[ind_pher] == max_phen )
                new_pos_ant.x -= 1;
            else if ( phen( new_pos_ant.x + 1, new_pos_ant.y )[ind_pher] == max_phen )
                new_pos_ant.x += 1;
            else if ( phen( new_pos_ant.x, new_pos_ant.y - 1 )[ind_pher] == max_phen )
                new_pos_ant.y -= 1;
            else  // if (phen(new_pos_ant.first,new_pos_ant.second+1)[ind_pher] == max_phen)
                new_pos_ant.y += 1;

            if (!is_inside(new_pos_ant) || phen[new_pos_ant][ind_pher] == -1) {
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
    }
}
