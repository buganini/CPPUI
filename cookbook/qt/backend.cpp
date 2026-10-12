#include "backend.hpp"

namespace Cookbook { namespace Backend {
    void configureRecipes(std::vector<Recipe> & pages) {
        for(auto & page: pages) {
            if(std::string(page.key) == "progressbar") {
                page.render = progressbar;
                page.remaining = "Spacer";
            }
        }
    }
}}
