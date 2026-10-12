#include "backend.hpp"

namespace Cookbook { namespace Backend {
    void configureRecipes(std::vector<Recipe> & pages) {
        for(auto & page: pages) {
            if(std::string(page.key) == "progressbar") {
                page.render = progressbar;
                page.remaining = "";
            } else if(std::string(page.key) == "text") {
                page.render = text;
                page.remaining = "";
            } else if(std::string(page.key) == "splitter") {
                page.render = splitter;
                page.remaining = "";
            } else if(std::string(page.key) == "canvas") {
                page.render = canvas;
                page.remaining = "";
            } else if(std::string(page.key) == "tab") {
                page.render = tab;
                page.remaining = "";
            }
        }
    }
}}
