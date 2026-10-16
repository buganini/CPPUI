#include "cppui/lvgl/combobox.hpp"
#include "cppui/lvgl/window.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    lv_obj_t * dropdown(lv_obj_t * object) {
        return lv_obj_check_type(object, &lv_dropdown_class) ? object : lv_obj_get_child(object, 1);
    }
    void choose(lv_obj_t * object, int index) {
        auto list = dropdown(object); lv_dropdown_set_selected(list, index);
        lv_obj_send_event(list, LV_EVENT_VALUE_CHANGED, nullptr);
    }
    std::string selected(lv_obj_t * object) {
        auto list = dropdown(object);
        if(lv_dropdown_get_text(list)) return lv_dropdown_get_text(list);
        char label[100]; lv_dropdown_get_selected_str(list, label, sizeof(label)); return label;
    }
    class Items: public CPPUI::LVGL::View {
        public:
        CPPUI::State<int> count{2};
        std::string label = "Duplicate";
        void content() override {
            for(int i = 0; i < static_cast<int>(count); ++i)
                CPPUI::ComboBoxItem(label, i ? "" : "A").id(std::to_string(i));
        }
    };
    class Root: public CPPUI::LVGL::View {
        public:
        bool show = true, editable = false, disabled = false, boundIndex = false, boundText = false, scoped = false, reversed = false;
        CPPUI::State<int> index{-1}; CPPUI::State<std::string> text{std::string("unknown")};
        int indexWrites = 0, textWrites = 0, changes = 0, version = 1;
        std::function<void()> action;
        std::shared_ptr<int> capture = std::make_shared<int>(1);
        void content() override {
            for(auto window: CPPUI::Window()) if(show) {
                CPPUI::Optional<CPPUI::Binding<int>> indices;
                CPPUI::Optional<CPPUI::Binding<std::string>> texts;
                if(boundIndex) indices = CPPUI::Binding<int>([this](){ return static_cast<int>(index); },
                    [this](int value){ ++indexWrites; index=value; auto invoke=action; if(invoke) invoke(); });
                if(boundText) texts = CPPUI::Binding<std::string>([this](){ return static_cast<std::string>(text); },
                    [this](const std::string & value){ ++textWrites; text=value; });
                auto combo = new CPPUI::LVGL::ComboBox(editable, indices, texts);
                if(disabled) combo->enabled(false);
                const auto current=version; auto token=capture;
                combo->change([this,current,token](){changes+=current;});
                for(auto scope:*combo) {
                    if(scoped) { (new Items)->id("left"); (new Items)->id("right"); }
                    else for(int i: reversed ? std::vector<int>{2,1,0} : std::vector<int>{0,1,2}) {
                        const char * labels[]={"Duplicate","Duplicate","Taipei (TPE)"};
                        const char * values[]={"A","","TPE"};
                        CPPUI::ComboBoxItem(labels[i],values[i]).id(std::to_string(i));
                    }
                }
            }
        }
        CPPUI::LVGL::ComboBox * combo() { return static_cast<CPPUI::LVGL::ComboBox *>(children->front()->children->front()); }
        lv_obj_t * native() { return static_cast<lv_obj_t *>(combo()->ui); }
    };
}
int main() {
    lv_init(); lv_display_create(800,600); auto group=lv_group_create(); lv_group_set_default(group);
    int result=0;
    try {
        {
            Root root; root.redraw(); auto object=root.native();
            require(selected(object).empty(), "unbound combo did not start empty");
            uint32_t enter=LV_KEY_ENTER;
            lv_obj_send_event(object,LV_EVENT_KEY,&enter);lv_obj_send_event(object,LV_EVENT_KEY,&enter);
            require(selected(object)=="Duplicate" && root.changes==1,"keyboard could not accept initial option");
            choose(object,1); require(selected(object)=="Duplicate" && root.changes==2, "unbound selection failed");
            root.reversed=true;root.redraw();
            require(root.native()==object && lv_dropdown_get_selected(object)==1,"keyed reorder lost selection");
            root.disabled=true;root.redraw();require(lv_obj_has_state(object,LV_STATE_DISABLED),"disable omitted");
            root.disabled=false;root.redraw();require(!lv_obj_has_state(object,LV_STATE_DISABLED),"omitted enabled did not reset");
            root.boundText=true;root.text=std::string();root.redraw();
            require(lv_dropdown_get_selected(object)==1 && root.changes==2,"empty raw value lost duplicate identity or emitted change");
            root.text=std::string("missing");require(selected(object).empty(),"unknown readonly value selected an item");
            choose(object,0);require(static_cast<std::string>(root.text)=="TPE" && root.textWrites==1,"selection stored label instead of raw value");
            root.boundIndex=true;root.index=2;root.redraw();require(lv_dropdown_get_selected(object)==2,"index binding did not take precedence");
            root.disabled=true;root.redraw();root.index=0;
            require(lv_dropdown_get_selected(object)==0 && root.indexWrites==0,"disabled model update was ignored or wrote back");
            root.disabled=false;root.version=3;root.redraw();const int before=root.changes;
            choose(object,2);require(root.indexWrites==1 && root.textWrites==2 && root.changes==before+3,"redraw retained stale callbacks or duplicate writes");
            std::weak_ptr<int> token=root.capture;root.capture.reset();root.redraw();
            root.action=[&](){root.show=false;root.redraw();};
            choose(object,1);require(token.expired() && root.textWrites==3 && static_cast<std::string>(root.text).empty(),"self-removal lost final value or retained callbacks");
        }
        {
            Root root;root.editable=true;root.boundIndex=true;root.boundText=true;root.redraw();
            auto object=root.native();auto editor=lv_obj_get_child(object,0);
            lv_textarea_set_text(editor,"tai");
            require(static_cast<int>(root.index)==-1 && static_cast<std::string>(root.text)=="tai"
                && lv_dropdown_get_option_count(dropdown(object))==1 && root.textWrites==1,"typed text/filter binding failed");
            choose(object,0);
            require(static_cast<int>(root.index)==2 && static_cast<std::string>(root.text)=="TPE"
                && std::string(lv_textarea_get_text(editor))=="Taipei (TPE)" && root.textWrites==2,"filtered selection lost original index/value");
            root.boundText=false;root.redraw();lv_textarea_set_text(editor,"free text");root.redraw();
            require(std::string(lv_textarea_get_text(editor))=="free text","index-only redraw lost editable buffer");
            lv_dropdown_open(dropdown(object));
            require(lv_dropdown_get_option_count(dropdown(object))==3,"arrow did not restore all choices after unmatched input");
            lv_obj_delete(object);require(!root.combo()->ui,"native deletion did not invalidate wrapper");
        }
        {
            Root root;root.scoped=true;root.redraw();auto object=root.native();
            auto left=static_cast<Items *>(root.combo()->children->at(0));
            auto right=static_cast<Items *>(root.combo()->children->at(1));
            choose(object,3);left->count=0;
            require(lv_dropdown_get_option_count(object)==2 && lv_dropdown_get_selected(object)==1,"independent empty view did not remove options/preserve scoped identity");
            right->label="Renamed";right->redraw();require(selected(object)=="Renamed","independent item rename lost selection");
            right->count=0;require(lv_obj_has_state(object,LV_STATE_DISABLED) && selected(object).empty(),"empty combo retained options/selection");
        }
    } catch(const std::exception & error) { std::cerr<<error.what()<<'\n';result=1; }
    lv_group_set_default(nullptr);lv_group_delete(group);lv_deinit();return result;
}
