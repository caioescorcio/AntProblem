#include "ant.hpp"
#include <algorithm>
#include "rand_generator.hpp"

void advance_ant( pheronome& phen_food,
                  pheronome& phen_nest,
                  const fractal_land& land,
                  const position_t& pos_food,
                  const position_t& pos_nest,
                  std::size_t& cpteur_food,
                  std::size_t& seed,
                  int& x,
                  int& y,
                  uint8_t& state,
                  double eps,
                  std::vector<std::size_t>& touched_food,
                  std::vector<std::size_t>& touched_nest )
{
    auto ant_choice = [&seed]() { return rand_double( 0.0, 1.0, seed ); };
    auto dir_choice = [&seed]() { return rand_int32( 1, 4, seed ); };
    double consumed_time = 0.0;
    const int dim = static_cast<int>(land.dimensions());

    auto cell_value = [&](const pheronome& phen, int cx, int cy) -> double {
        if (cx < 0 || cy < 0 || cx >= dim || cy >= dim) {
            return -1.0;
        }
        return phen(static_cast<pheronome::size_t>(cx), static_cast<pheronome::size_t>(cy));
    };

    while (consumed_time < 1.0) {
        pheronome& phen = (state == 0) ? phen_food : phen_nest;
        double choix = ant_choice();
        int new_x = x;
        int new_y = y;

        double left  = cell_value( phen, new_x - 1, new_y );
        double right = cell_value( phen, new_x + 1, new_y );
        double up    = cell_value( phen, new_x, new_y - 1 );
        double down  = cell_value( phen, new_x, new_y + 1 );
        double max_phen = std::max( std::max( left, right ), std::max( up, down ) );

        if ( ( choix > eps ) || ( max_phen <= 0.0 ) ) {
            do {
                new_x = x;
                new_y = y;
                int d = dir_choice();
                if ( d == 1 ) new_x -= 1;
                if ( d == 2 ) new_y -= 1;
                if ( d == 3 ) new_x += 1;
                if ( d == 4 ) new_y += 1;
            } while ( cell_value( phen, new_x, new_y ) == -1.0 );
        } else {
            if ( left == max_phen )
                new_x -= 1;
            else if ( right == max_phen )
                new_x += 1;
            else if ( up == max_phen )
                new_y -= 1;
            else
                new_y += 1;
        }

        consumed_time += land(static_cast<unsigned long>(new_x), static_cast<unsigned long>(new_y));
        x = new_x;
        y = new_y;

        position_t pos{ x, y };
        touched_food.push_back( phen_food.linear_index(pos) );
        touched_nest.push_back( phen_nest.linear_index(pos) );

        if ( x == pos_nest.x && y == pos_nest.y ) {
            if ( state != 0 ) {
                cpteur_food += 1;
            }
            state = 0;
        }
        if ( x == pos_food.x && y == pos_food.y ) {
            state = 1;
        }
    }
}
