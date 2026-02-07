# ifndef _POPULATION_HPP_
# define _POPULATION_HPP_
# include <utility>
# include "fractal_land.hpp"
# include "pheronome.hpp"
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

        Population(int capacity) : m_size(0) { // start with 0 active ants
            positions.reserve(capacity);
            states.reserve(capacity);
            seeds.reserve(capacity);
            m_eps = 0;
        }

        ~Population() = default;

        void new_ant(position_t pos, state st, std::size_t seed){   // returns void because its id is not important
            positions.push_back(pos);
            states.push_back(st);
            seeds.push_back(seed);
            m_size++;
        }
        
        void clone_ant(int index) {
            new_ant(positions[index], states[index], seeds[index]);
        }

        void destroy_ant(int index) {
            if (index >= (int)this->positions.size()) return;

            positions[index] = positions.back();        // vector[index] = vector[n-1] 
            states[index] = states.back();
            seeds[index] = seeds.back();

            positions.pop_back();    // vector[n-1] is popped
            states.pop_back();
            seeds.pop_back();
            m_size--;
        }

        void set_loaded(int index) { states[index] = loaded; }
        void unset_loaded(int index) { states[index] = unloaded; }

        bool is_loaded(int index) const { return states[index] == loaded; }

        const position_t& get_position(int index) const { return this->positions[index]; }

        void set_exploration_coef(double eps) { this->m_eps = eps; }   // set commom exploration coefficient to the given value

        size_t get_size() const { return this->m_size; }

        void advance( int index, pheronome& phen, const fractal_land& land,
                    const position_t& pos_food, const position_t& pos_nest, std::size_t& cpteur_food );

        void reserve(size_t capacity) {
            positions.reserve(capacity);
            states.reserve(capacity);
            seeds.reserve(capacity);
        }
    };

#endif