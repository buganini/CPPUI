#ifndef CPPUI_QT_FONTCOMBOBOX
#define CPPUI_QT_FONTCOMBOBOX
#include "base.hpp"
#include "../binding.hpp"
#include "../font_choices.hpp"
#include <QFontComboBox>
#include <QSignalBlocker>
namespace CPPUI { namespace Qt {
    class FontComboBox: public BaseWidget {
        Binding<std::string> model;
        std::string value;
        Optional<std::vector<std::string>> fonts;
        bool isDisabled=false;
        Callback onChange;
    public:
        FontComboBox(Binding<std::string> model,Optional<std::vector<std::string>> fonts={}):model(model),value(model.get()),fonts(std::move(fonts)) {}
        std::size_t typecode() const override {return BaseWidget::typecode() ^ (fonts ? 0xF017U : 0U);}
        FontComboBox & disabled(bool value=true) {isDisabled=value;return *this;}
        FontComboBox & change(Callback callback) {onChange=std::move(callback);return *this;}
        void update(BaseWidget * previous) override {
            ui=previous && previous->ui ? previous->ui : fonts ? static_cast<QWidget *>(new Widget<QComboBox>()) : new Widget<QFontComboBox>();
            auto combo=static_cast<QComboBox *>(ui);
            {
                QSignalBlocker block(combo);
                combo->setEditable(false);
                if(fonts) {
                    QStringList choices;for(const auto & font:*fonts) choices.append(QString::fromStdString(font));
                    QStringList existing;for(int i=0;i<combo->count();++i) existing.append(combo->itemText(i));
                    if(existing!=choices) {combo->clear();combo->addItems(choices);}
                }
                combo->setCurrentIndex(combo->findText(QString::fromStdString(value),::Qt::MatchExactly));
                combo->setDisabled(isDisabled);
            }
            auto binding=model;auto callback=onChange;
            connectSignal(QObject::connect(combo,QOverload<int>::of(&QComboBox::activated),combo,[combo,binding,callback](int index) mutable {
                if(!combo->isEnabled() || index<0 || index>=combo->count()) return;
                const auto family=combo->itemText(index).toStdString();
                auto current=binding;auto invoke=callback;
                try {current.set(family);if(invoke)invoke();}
                catch(const std::exception & e){std::cerr<<"CPPUI FontComboBox: "<<e.what()<<'\n';}
                catch(...){std::cerr<<"CPPUI FontComboBox: unknown exception\n";}
            }));
        }
    };
}}
namespace CPPUI {
    inline Qt::FontComboBox & FontComboBox(Binding<std::string> model) {return *new Qt::FontComboBox(model);}
    inline Qt::FontComboBox & FontComboBox(Binding<std::string> model,const std::vector<std::string> & fonts) {return *new Qt::FontComboBox(model,detail::fontChoices(fonts));}
}
#endif
