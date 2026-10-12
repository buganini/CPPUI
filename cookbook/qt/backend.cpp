#include "backend.hpp"

namespace Cookbook { namespace Backend {
    void configureRecipes(std::vector<Recipe> & pages) {
        for(auto & page: pages) {
            if(std::string(page.key) == "progressbar") {
                page.render = progressbar;
                page.remaining = "";
            } else if(std::string(page.key) == "checkbox") {
                page.render = checkbox;
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
            } else if(std::string(page.key) == "dialog") {
                page.render = dialog;
                page.remaining = "";
            } else if(std::string(page.key) == "timeline") {
                page.render = timeline;
                page.remaining = "";
            } else if(std::string(page.key) == "grid") {
                page.render = grid;
                page.remaining = "";
            } else if(std::string(page.key) == "stack") {
                page.render = stack;
                page.remaining = "";
            } else if(std::string(page.key) == "image") {
                page.render = image;
                page.remaining = "";
            } else if(std::string(page.key) == "mdi") {
                page.render = mdi;
                page.remaining = "";
            } else if(std::string(page.key) == "modal") {
                page.render = modal;
                page.remaining = "";
            }
        }
    }
}}
