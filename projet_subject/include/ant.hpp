#ifndef _ANT_HPP_
#define _ANT_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>
#include "basic_types.hpp"
#include "fractal_land.hpp"
#include "pheronome.hpp"

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
                  std::vector<std::size_t>& touched_nest );

#endif
