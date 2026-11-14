#include "../view.hpp"
#include "../color.hpp"
#include "adaptive.hpp"
#include "font.hpp"

#include <lvgl.h>
#include <cstdio>
#include <cstring>
#include <memory>

#ifndef CPPUI_LVGL_BASE
#define CPPUI_LVGL_BASE

namespace CPPUI {
    namespace LVGL {
        constexpr char backendName[] = "LVGL";

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
                using Clock=std::chrono::steady_clock;
                struct Entry { Clock::time_point deadline; Callback callback; };
                struct Pending { std::mutex mutex; std::vector<Entry> callbacks; };
                std::shared_ptr<Pending> pending{new Pending()};
                lv_timer_t * timer;
                Commands():timer(lv_timer_create([](lv_timer_t * timer) {
                    auto pending=static_cast<Commands *>(lv_timer_get_user_data(timer))->pending;
                    std::vector<std::function<void()>> callbacks;
                    { std::lock_guard<std::mutex> lock(pending->mutex);
                      auto & entries=pending->callbacks;
                      std::vector<Entry> waiting;
                      const auto now=Clock::now();
                      for(auto & entry:entries) {
                          if(entry.deadline<=now) callbacks.push_back(std::move(entry.callback));
                          else waiting.push_back(std::move(entry));
                      }
                      entries.swap(waiting);
                    }
                    for(auto & callback:callbacks) try { callback(); }
                    catch(const std::exception & error) { std::fprintf(stderr,"CPPUI ref: %s\n",error.what()); }
                    catch(...) { std::fprintf(stderr,"CPPUI ref failed\n"); }
                },LV_DEF_REFR_PERIOD,this)) {}
                ~Commands() { lv_timer_delete(timer); }
            };
            std::unique_ptr<Commands> commands;
            friend class Tabs;
            friend class Box;
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
            std::vector<SavedStyle> modifiedStyles, boxStyles;
            void restoreBoxStyles() {
                if(!ui) return;
                auto object=static_cast<lv_obj_t *>(ui);
                for(const auto & entry:boxStyles) {
                    if(entry.value) lv_obj_set_local_style_prop(object,entry.property,*entry.value,0);
                    else lv_obj_remove_local_style_prop(object,entry.property,0);
                }
                boxStyles.clear();
            }
            void saveBoxStyle(lv_style_prop_t property) {
                lv_style_value_t value;
                const bool local=lv_obj_get_local_style_prop(static_cast<lv_obj_t *>(ui),property,&value,0)==LV_STYLE_RES_FOUND;
                boxStyles.push_back({property,local ? Optional<lv_style_value_t>(value) : Optional<lv_style_value_t>()});
            }
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
                if(minimumSize(true)) { saveStyle(LV_STYLE_MIN_WIDTH); lv_obj_set_style_min_width(object,*minimumSize(true),0); }
                if(minimumSize(false)) { saveStyle(LV_STYLE_MIN_HEIGHT); lv_obj_set_style_min_height(object,*minimumSize(false),0); }
                if(layout_weight) { saveStyle(LV_STYLE_FLEX_GROW); lv_obj_set_flex_grow(object, *layout_weight); }
                if(layout_padding) {
                    for(auto property: {LV_STYLE_PAD_TOP, LV_STYLE_PAD_RIGHT, LV_STYLE_PAD_BOTTOM, LV_STYLE_PAD_LEFT}) saveStyle(property);
                    const auto & p = *layout_padding;
                    lv_obj_set_style_pad_top(object, p.top, 0); lv_obj_set_style_pad_right(object, p.right, 0);
                    lv_obj_set_style_pad_bottom(object, p.bottom, 0); lv_obj_set_style_pad_left(object, p.left, 0);
                }
                if(layout_margin) {
                    for(auto property: {LV_STYLE_MARGIN_TOP, LV_STYLE_MARGIN_RIGHT, LV_STYLE_MARGIN_BOTTOM, LV_STYLE_MARGIN_LEFT}) saveStyle(property);
                    const auto & m = *layout_margin;
                    lv_obj_set_style_margin_top(object, m.top, 0); lv_obj_set_style_margin_right(object, m.right, 0);
                    lv_obj_set_style_margin_bottom(object, m.bottom, 0); lv_obj_set_style_margin_left(object, m.left, 0);
                }
                if(style_color) { saveStyle(LV_STYLE_TEXT_COLOR); saveStyle(LV_STYLE_TEXT_OPA); lv_obj_set_style_text_color(object, lv_color_hex(*style_color), 0); lv_obj_set_style_text_opa(object,colorAlpha(*style_color),0); }
                if(style_bgcolor) {
                    saveStyle(LV_STYLE_BG_COLOR); saveStyle(LV_STYLE_BG_OPA);
                    lv_obj_set_style_bg_color(object, lv_color_hex(*style_bgcolor), 0);
                    lv_obj_set_style_bg_opa(object, colorAlpha(*style_bgcolor), 0);
                }
                if(style_fontsize || style_fontweight) {
                    // LVGL uses configured bitmap fonts; choose the closest enabled
                    // Montserrat size to the requested point size at 96 dpi.
                    const auto font = resolveFont();
                    saveStyle(LV_STYLE_TEXT_FONT); lv_obj_set_style_text_font(object, font, 0);
                }
            }
            const lv_font_t * resolveFont() const;
            const lv_font_t * resolveFontSize() const;
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
            bool hasClickHandler = false;
            protected:
            // Always clickable native controls (buttons, editors, ...). As in
            // PUI, every other widget accepts input only while a click handler
            // is set; otherwise presses fall through to its parents.
            bool clickable = false;
            virtual bool acceptsClicks() const { return clickable || hasClickHandler; }
            virtual void nativeEvent(lv_event_t * event) {
                if(acceptsClicks() && lv_event_get_code(event) == LV_EVENT_CLICKED) {
                    auto invoke = onClicked;
                    if(invoke) invoke();
                }
            }
            public:
            int expandX=0,expandY=0;
            int baseExpansion(bool x) const override { return x ? expandX : expandY; }
            ~Node() override { clearRef(); unobserve(); }
            void click(std::function<void()> callback) override {
                hasClickHandler = bool(callback);
                ::CPPUI::Node::click(std::move(callback));
            }
            CommandPoster commandPoster() override {
                if(!ui) return {};
                if(!commands) commands.reset(new Commands());
                auto pending=commands->pending;
                return [pending](std::function<void()> callback) {
                    std::lock_guard<std::mutex> lock(pending->mutex);
                    pending->callbacks.push_back({Commands::Clock::now(),std::move(callback)});
                };
            }
            TimerPoster timerPoster() override {
                if(!ui) return {};
                if(!commands) commands.reset(new Commands());
                auto pending=commands->pending;
                return [pending](int delay,Callback callback) {
                    std::lock_guard<std::mutex> lock(pending->mutex);
                    pending->callbacks.push_back({Commands::Clock::now()+std::chrono::milliseconds(delay),std::move(callback)});
                };
            }
            void update(::CPPUI::Node * previous) override {
                auto prev = static_cast<Node *>(previous);
                if(prev) { prev->restoreBoxStyles(); prev->restoreOptions(); commands=std::move(prev->commands); }
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
                if(object) lv_obj_set_clickable(object, acceptsClicks());
                if(object) {
                    applyOptions();
                    if(supportsVisibility() && (!is_visible || (prev && !prev->is_visible))) lv_obj_set_hidden(object,!is_visible);
                }
                ::CPPUI::Node::update(previous);
            }

            std::unique_ptr<detail::SizeObserver> observeSize() override {
                return ui ? std::unique_ptr<detail::SizeObserver>(new SizeObserver(static_cast<lv_obj_t *>(ui),sizeState)) : nullptr;
            }
            virtual void layoutChild(Node *) {}
            void postUpdate() override {
                if(ui && !layoutVisible()) lv_obj_set_hidden(static_cast<lv_obj_t *>(ui),true);
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

            void addChild(int idx, ::CPPUI::Node *child) override {
                addChild(idx, (Node *) child);
            }

            void removeChild(int idx, ::CPPUI::Node *child) override {
                removeChild(idx, (Node *) child);
            }

            void destroy(bool direct) override {
                clearRef(); commands.reset();
                unobserve();
                if(direct && ui){
                    lv_obj_delete((lv_obj_t *)ui);
                }
                ui = nullptr;
            }
         };
        class Layout: public Node {
            protected:
            static bool fillsAxis(lv_obj_t * object,bool horizontal) {
                const auto size=horizontal ? lv_obj_get_style_width(object,LV_PART_MAIN) : lv_obj_get_style_height(object,LV_PART_MAIN);
                if(size!=LV_SIZE_CONTENT) return true;
                auto owner=lv_obj_get_parent(object);
                if(!owner) return false;
                const auto layout=lv_obj_get_style_layout(owner,LV_PART_MAIN);
#if LV_USE_FLEX
                if(layout==LV_LAYOUT_FLEX && lv_obj_get_style_flex_grow(object,LV_PART_MAIN)>0) {
                    const bool row=(lv_obj_get_style_flex_flow(owner,LV_PART_MAIN)&LV_FLEX_COLUMN)==0;
                    if(row==horizontal) return true;
                }
#endif
#if LV_USE_GRID
                if(layout==LV_LAYOUT_GRID) {
                    const auto align=horizontal ? lv_obj_get_style_grid_cell_x_align(object,LV_PART_MAIN) : lv_obj_get_style_grid_cell_y_align(object,LV_PART_MAIN);
                    if(align==LV_GRID_ALIGN_STRETCH) return true;
                }
#endif
                return false;
            }
        };

        inline const lv_font_t * Node::resolveFont() const {
            const auto inherited=lv_obj_get_style_text_font(static_cast<lv_obj_t *>(ui),LV_PART_MAIN);
            const bool bold=style_fontweight ? *style_fontweight>=600 : Fonts::isBoldFont(inherited);
            return Fonts::withFontWeight(resolveFontSize(),bold);
        }
        inline const lv_font_t * Node::resolveFontSize() const {
            // Only the compiled Montserrat faces exist; like PUI, fontFamily
            // cannot select other fonts and is ignored.
            if(!style_fontsize) return lv_obj_get_style_text_font(static_cast<lv_obj_t *>(ui),LV_PART_MAIN);
            const lv_font_t * best = nullptr;
            const int pixels = (*style_fontsize * 4 + 1) / 3;
            int distance = 0;
            const auto consider = [&](const lv_font_t * font, int size) {
                if(!best || std::abs(size - pixels) < distance) { best = font; distance = std::abs(size - pixels); }
            };
#if LV_FONT_MONTSERRAT_8
            consider(&lv_font_montserrat_8, 8);
#endif
#if LV_FONT_MONTSERRAT_10
            consider(&lv_font_montserrat_10, 10);
#endif
#if LV_FONT_MONTSERRAT_12
            consider(&lv_font_montserrat_12, 12);
#endif
#if LV_FONT_MONTSERRAT_14
            consider(&lv_font_montserrat_14, 14);
#endif
#if LV_FONT_MONTSERRAT_16
            consider(&lv_font_montserrat_16, 16);
#endif
#if LV_FONT_MONTSERRAT_18
            consider(&lv_font_montserrat_18, 18);
#endif
#if LV_FONT_MONTSERRAT_20
            consider(&lv_font_montserrat_20, 20);
#endif
#if LV_FONT_MONTSERRAT_22
            consider(&lv_font_montserrat_22, 22);
#endif
#if LV_FONT_MONTSERRAT_24
            consider(&lv_font_montserrat_24, 24);
#endif
#if LV_FONT_MONTSERRAT_26
            consider(&lv_font_montserrat_26, 26);
#endif
#if LV_FONT_MONTSERRAT_28
            consider(&lv_font_montserrat_28, 28);
#endif
#if LV_FONT_MONTSERRAT_30
            consider(&lv_font_montserrat_30, 30);
#endif
#if LV_FONT_MONTSERRAT_32
            consider(&lv_font_montserrat_32, 32);
#endif
#if LV_FONT_MONTSERRAT_34
            consider(&lv_font_montserrat_34, 34);
#endif
#if LV_FONT_MONTSERRAT_36
            consider(&lv_font_montserrat_36, 36);
#endif
#if LV_FONT_MONTSERRAT_38
            consider(&lv_font_montserrat_38, 38);
#endif
#if LV_FONT_MONTSERRAT_40
            consider(&lv_font_montserrat_40, 40);
#endif
#if LV_FONT_MONTSERRAT_42
            consider(&lv_font_montserrat_42, 42);
#endif
#if LV_FONT_MONTSERRAT_44
            consider(&lv_font_montserrat_44, 44);
#endif
#if LV_FONT_MONTSERRAT_46
            consider(&lv_font_montserrat_46, 46);
#endif
#if LV_FONT_MONTSERRAT_48
            consider(&lv_font_montserrat_48, 48);
#endif
            (void)consider;
            return best ? best : LV_FONT_DEFAULT;
        }
    }
}

#endif
