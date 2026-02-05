#ifndef _ANT_HPP_
# define _ANT_HPP_
# include <utility>
# include "fractal_land.hpp"
# include "basic_types.hpp"

enum state { unloaded = 0, loaded = 1 };

class Population {
    private:
        int m_size;
        std::vector<position_t> positions;
        std::vector<state> states;
        std::vector<std::size_t> seeds;
        double m_eps;
    
    public:
        void Population(int size) : m_size(size) {
            positions.resize(m_size);
            states.resize(m_size, unloaded); // start all ants with unloaded state
            seeds.resize(m_size);
            m_eps = 0;
        }

        ~Population() = default;

        void new_ant(position_t pos, state st, std::size_t seed){   // returns void because its id is not important
            positions.push_back(pos);
            states.push_back(st);
            seeds.push_back(seed);
        }
        
        void clone_ant(int index) {
            new_ant(positions[index], states[index], seeds[index]);
        }

        void destroy_ant(int index) {
            if (index >= posistions.size()) return;

            positions[index] = positions.back();        // vector[index] = vector[n-1] 
            states[index] = states.back();
            seeds[index] = seeds.back();

            positions[index] = positions.pop_back();    // vector[n-1] is popped
            states[index] = states.pop_back();
            seeds[index] = seeds.pop_back();
        }

        void set_loaded(int index) { states[index] = loaded; }
        void unset_loaded(int index) { states[index] = unloaded; }

        bool is_loaded(int index) const { return states[index] == loaded; }

        position_t& get_position(int index) const { return positions[index]; }

        static void set_exploration_coef(double eps) { m_eps = eps; }   // set commom exploration coefficient to the given value

        void advance( int index, pheronome& phen, const fractal_land& land,
                    const position_t& pos_food, const position_t& pos_nest, std::size_t& cpteur_food );
    };