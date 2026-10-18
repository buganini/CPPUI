#include "../view.hpp"

#include <lvgl.h>
#include <cstdio>
#include <memory>

#ifndef CPPUI_LVGL_BASE
#define CPPUI_LVGL_BASE

namespace CPPUI {
    namespace LVGL {
        class View: public ::CPPUI::View {
            public:
            ~View() override {
                destroyed = true;
                for(auto child: *children) recur_delete(child, true);
                children->clear();
            }
            void destroy(bool) override {
                // Virtual views own declarations, not a native object.
                ui = nullptr;
            }
         };

        class Node: public ::CPPUI::Node {
            struct Commands {
                struct Pending { std::mutex mutex; std::vector<std::function<void()>> callbacks; };
                std::shared_ptr<Pending> pending{new Pending()};
                lv_timer_t * timer;
                Commands():timer(lv_timer_create([](lv_timer_t * timer) {
                    auto pending=static_cast<Commands *>(lv_timer_get_user_data(timer))->pending;
                    std::vector<std::function<void()>> callbacks;
                    { std::lock_guard<std::mutex> lock(pending->mutex); callbacks.swap(pending->callbacks); }
                    for(auto & callback:callbacks) try { callback(); }
                    catch(const std::exception & error) { std::fprintf(stderr,"CPPUI ref: %s\n",error.what()); }
                    catch(...) { std::fprintf(stderr,"CPPUI ref failed\n"); }
                },LV_DEF_REFR_PERIOD,this)) {}
                ~Commands() { lv_timer_delete(timer); }
            };
            std::unique_ptr<Commands> commands;
            friend class Tabs;
            struct Observer {
                lv_obj_t * object;
                Node * owner;
                Observer ** bookmark = nullptr;
                void invalidate() {
                    object = nullptr;
                    owner = nullptr;
                    if(bookmark) { *bookmark = nullptr; bookmark = nullptr; }
                }
                Observer(lv_obj_t * object, Node * owner): object(object), owner(owner) {}
            };
            std::unique_ptr<Observer> observed;
            struct SavedStyle { lv_style_prop_t property; Optional<lv_style_value_t> value; };
            std::vector<SavedStyle> modifiedStyles;
            void saveStyle(lv_style_prop_t property) {
                lv_style_value_t value;
                const bool local = lv_obj_get_local_style_prop(static_cast<lv_obj_t *>(ui), property, &value, 0) == LV_STYLE_RES_FOUND;
                modifiedStyles.push_back({property, local ? Optional<lv_style_value_t>(value) : Optional<lv_style_value_t>()});
            }
            void restoreOptions() {
                if(!ui) return;
                auto object = static_cast<lv_obj_t *>(ui);
                for(const auto & entry: modifiedStyles) {
                    if(entry.value) lv_obj_set_local_style_prop(object, entry.property, *entry.value, 0);
                    else lv_obj_remove_local_style_prop(object, entry.property, 0);
                }
                modifiedStyles.clear();
            }
            void applyOptions() {
                auto object = static_cast<lv_obj_t *>(ui);
                if(layout_width) { saveStyle(LV_STYLE_WIDTH); lv_obj_set_width(object, *layout_width); }
                if(layout_height) { saveStyle(LV_STYLE_HEIGHT); lv_obj_set_height(object, *layout_height); }
                if(layout_weight) { saveStyle(LV_STYLE_FLEX_GROW); lv_obj_set_flex_grow(object, *layout_weight); }
                if(layout_padding) {
                    for(auto property: {LV_STYLE_PAD_TOP, LV_STYLE_PAD_RIGHT, LV_STYLE_PAD_BOTTOM, LV_STYLE_PAD_LEFT}) saveStyle(property);
                    const auto & p = *layout_padding;
                    lv_obj_set_style_pad_top(object, p.top, 0); lv_obj_set_style_pad_right(object, p.right, 0);
                    lv_obj_set_style_pad_bottom(object, p.bottom, 0); lv_obj_set_style_pad_left(object, p.left, 0);
                }
                if(style_color) { saveStyle(LV_STYLE_TEXT_COLOR); lv_obj_set_style_text_color(object, lv_color_hex(*style_color), 0); }
                if(style_bgcolor) {
                    saveStyle(LV_STYLE_BG_COLOR); saveStyle(LV_STYLE_BG_OPA);
                    lv_obj_set_style_bg_color(object, lv_color_hex(*style_bgcolor), 0);
                    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
                }
                if(style_fontsize || style_fontfamily) {
                    // LVGL uses configured bitmap fonts; choose the closest enabled
                    // Montserrat size to the requested point size at 96 dpi.
                    const auto font = resolveFont();
                    saveStyle(LV_STYLE_TEXT_FONT); lv_obj_set_style_text_font(object, font, 0);
                }
            }
            const lv_font_t * resolveFont() const;
            static void event(lv_event_t * event) {
                auto observer = static_cast<Observer *>(lv_event_get_user_data(event));
                auto node = observer->owner;
                if(lv_event_get_code(event) == LV_EVENT_DELETE) {
                    node->clearRef();
                    node->ui = nullptr;
                    observer->invalidate();
                } else {
                    try { node->nativeEvent(event); }
                    catch(const std::exception & error) {
                        std::fprintf(stderr, "CPPUI LVGL event failed: %s\n", error.what());
                    } catch(...) {
                        std::fprintf(stderr, "CPPUI LVGL event failed: unknown exception\n");
                    }
                }
                // User code may have deleted both the declaration and observer.
            }
            void unobserve() {
                if(observed && observed->object)
                    lv_obj_remove_event_cb_with_user_data(observed->object, event, observed.get());
                if(observed) observed->invalidate();
                observed.reset();
            }
            protected:
            bool clickable = false;
            virtual void nativeEvent(lv_event_t * event) {
                if(clickable && lv_event_get_code(event) == LV_EVENT_CLICKED) {
                    auto invoke = onClicked;
                    if(invoke) invoke();
                }
            }
            public:
            ~Node() override { clearRef(); unobserve(); }
            CommandPoster commandPoster() override {
                if(!commands) commands.reset(new Commands());
                auto pending=commands->pending;
                return [pending](std::function<void()> callback) {
                    std::lock_guard<std::mutex> lock(pending->mutex);
                    pending->callbacks.push_back(std::move(callback));
                };
            }
            void update(::CPPUI::Node * previous) override {
                auto prev = static_cast<Node *>(previous);
                if(prev) { prev->restoreOptions(); commands=std::move(prev->commands); }
                update(prev);
                auto object = static_cast<lv_obj_t *>(ui);
                if(prev && prev->observed && prev->observed->object == object) {
                    // Keep the native event list unchanged during a callback that
                    // redraws and then removes itself (LVGL retains its storage).
                    observed = std::move(prev->observed);
                    observed->owner = this;
                } else {
                    if(prev) prev->unobserve();
                    if(object) {
                        observed.reset(new Observer(object, this));
                        lv_obj_add_event_cb(object, event, LV_EVENT_ALL, observed.get());
                    }
                }
                if(object && clickable) lv_obj_set_clickable(object, true);
                if(object) applyOptions();
                ::CPPUI::Node::update(previous);
            }

            virtual void layoutChild(Node *) {}
            void postUpdate() override {
                if(auto owner = dynamic_cast<Node *>(dom_parent)) owner->layoutChild(this);
                ::CPPUI::Node::postUpdate();
            }

            virtual void update(Node * prev) = 0;
            virtual void addChild(int idx, Node * child) {
                if(child->ui) lv_obj_move_to_index(static_cast<lv_obj_t *>(child->outer()), idx);
            }

            virtual void removeChild(int idx, Node * child) {
                if(child->ui) lv_obj_move_to_index(static_cast<lv_obj_t *>(child->outer()), -1);
            }

            void addChild(int idx, ::CPPUI::Node *child) {
                addChild(idx, (Node *) child);
            }

            void removeChild(int idx, ::CPPUI::Node *child) {
                removeChild(idx, (Node *) child);
            }

            void destroy(bool direct) {
                clearRef(); commands.reset();
                unobserve();
                if(direct && ui){
                    lv_obj_delete((lv_obj_t *)ui);
                }
                ui = nullptr;
            }
         };
        inline const lv_font_t * Node::resolveFont() const {
            if(style_fontfamily && !style_fontfamily->empty() && *style_fontfamily != "Montserrat")
                throw std::invalid_argument("LVGL style fontFamily supports only configured Montserrat fonts");
            const lv_font_t * best = LV_FONT_DEFAULT;
            if(!style_fontsize) return best;
            const int pixels = (*style_fontsize * 4 + 1) / 3;
            int distance = std::abs(int(best->line_height) - pixels);
#if LV_FONT_MONTSERRAT_8
            if(std::abs(8 - pixels) < distance) { best = &lv_font_montserrat_8; distance = std::abs(8 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_10
            if(std::abs(10 - pixels) < distance) { best = &lv_font_montserrat_10; distance = std::abs(10 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_12
            if(std::abs(12 - pixels) < distance) { best = &lv_font_montserrat_12; distance = std::abs(12 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_14
            if(std::abs(14 - pixels) < distance) { best = &lv_font_montserrat_14; distance = std::abs(14 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_16
            if(std::abs(16 - pixels) < distance) { best = &lv_font_montserrat_16; distance = std::abs(16 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_18
            if(std::abs(18 - pixels) < distance) { best = &lv_font_montserrat_18; distance = std::abs(18 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_20
            if(std::abs(20 - pixels) < distance) { best = &lv_font_montserrat_20; distance = std::abs(20 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_22
            if(std::abs(22 - pixels) < distance) { best = &lv_font_montserrat_22; distance = std::abs(22 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_24
            if(std::abs(24 - pixels) < distance) { best = &lv_font_montserrat_24; distance = std::abs(24 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_26
            if(std::abs(26 - pixels) < distance) { best = &lv_font_montserrat_26; distance = std::abs(26 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_28
            if(std::abs(28 - pixels) < distance) { best = &lv_font_montserrat_28; distance = std::abs(28 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_30
            if(std::abs(30 - pixels) < distance) { best = &lv_font_montserrat_30; distance = std::abs(30 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_32
            if(std::abs(32 - pixels) < distance) { best = &lv_font_montserrat_32; distance = std::abs(32 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_34
            if(std::abs(34 - pixels) < distance) { best = &lv_font_montserrat_34; distance = std::abs(34 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_36
            if(std::abs(36 - pixels) < distance) { best = &lv_font_montserrat_36; distance = std::abs(36 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_38
            if(std::abs(38 - pixels) < distance) { best = &lv_font_montserrat_38; distance = std::abs(38 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_40
            if(std::abs(40 - pixels) < distance) { best = &lv_font_montserrat_40; distance = std::abs(40 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_42
            if(std::abs(42 - pixels) < distance) { best = &lv_font_montserrat_42; distance = std::abs(42 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_44
            if(std::abs(44 - pixels) < distance) { best = &lv_font_montserrat_44; distance = std::abs(44 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_46
            if(std::abs(46 - pixels) < distance) { best = &lv_font_montserrat_46; distance = std::abs(46 - pixels); }
#endif
#if LV_FONT_MONTSERRAT_48
            if(std::abs(48 - pixels) < distance) { best = &lv_font_montserrat_48; distance = std::abs(48 - pixels); }
#endif
            return best;
        }
    }
}

#endif
