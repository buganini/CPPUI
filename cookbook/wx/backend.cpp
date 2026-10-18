#include "backend.hpp"

namespace Cookbook { namespace Backend {
    void configureRecipes(std::vector<Recipe> & pages) {
        for(auto & page: pages) {
            if(std::string(page.key) == "stack") {
                page.render=stack; page.remaining="";
            } else if(std::string(page.key) == "tree_adapter") {
                page.render = treeAdapter;
                page.remaining = "";
            } else if(std::string(page.key) == "tree") {
                page.render = tree;
                page.remaining = "";
            } else if(std::string(page.key) == "table_adapter") {
                page.render = tableAdapter;
                page.remaining = "";
            } else if(std::string(page.key) == "table") {
                page.render = table;
                page.remaining = "";
            } else if(std::string(page.key) == "progressbar") {
                page.render = progressbar;
                page.remaining = "";
            } else if(std::string(page.key) == "checkbox") {
                page.render = checkbox;
                page.remaining = "";
            } else if(std::string(page.key) == "heavycanvas") {
                page.render = heavycanvas; page.remaining = "";
            } else if(std::string(page.key) == "canvas") {
                page.render = canvas;
                page.remaining = "";
            } else if(std::string(page.key) == "tab") {
                page.render = tab;
                page.remaining = "";
            } else if(std::string(page.key) == "drop") {
                page.render=drop; page.remaining="";
            } else if(std::string(page.key) == "views") {
                page.render = views;
                page.remaining = "";
            } else if(std::string(page.key) == "dialog") {
                page.render = dialog;
                page.remaining = "";
            } else if(std::string(page.key) == "timeline") {
                page.render = timeline;
                page.remaining = "";
            } else if(std::string(page.key) == "scroll") {
                page.render = scroll;
                page.remaining = "";
            } else if(std::string(page.key) == "combobox") {
                page.render = combobox;
                page.remaining = "";
            } else if(std::string(page.key) == "radiobutton") {
                page.render = radiobutton;
                page.remaining = "";
            } else if(std::string(page.key) == "maskedfield") {
                page.render = maskedfield; page.remaining = "";
            } else if(std::string(page.key) == "textfield") {
                page.render = textfield;
                page.remaining = "String models; dynamic numeric conversion and typed input/change payloads pending";
            } else if(std::string(page.key) == "text") {
                page.render = text;
                page.remaining = "Plain multiline text and HTML implemented; Markdown displays raw source";
            } else if(std::string(page.key) == "grid") {
                page.render = grid;
                page.remaining = "";
            } else if(std::string(page.key) == "image") {
                page.render = image;
                page.remaining = "";
            }
            if(std::string(page.key)=="splitter") page.remaining="wx has two-pane splitters; shared API permits multiple panes";
            if(std::string(page.key)=="modal") page.remaining="Nonblocking application-modal ownership policy needed";
            if(std::string(page.key)=="mdi") page.remaining="MDI requires a native MDI parent frame";
        }
    }
}}
