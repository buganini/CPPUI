#ifndef CPPUI_APPLICATION_TITLE
#define CPPUI_APPLICATION_TITLE

#include "node.hpp"
#include "optional.hpp"

namespace CPPUI {
    class ApplicationTitle {
        public:
        virtual ~ApplicationTitle() = default;
        virtual const std::string & applicationTitle() const = 0;
    };
    inline OptionalText windowTitle(Node * node, const OptionalText & explicitTitle) {
        if(explicitTitle) return explicitTitle;
        while(node->parent && node->parent != node) node = node->parent;
        auto app = dynamic_cast<ApplicationTitle *>(node);
        return app && !app->applicationTitle().empty() ? OptionalText(app->applicationTitle()) : OptionalText();
    }
}
#endif
