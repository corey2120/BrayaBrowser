#include "TabGroup.h"
#include <algorithm>

TabGroup::TabGroup(const std::string& name, const std::string& color)
    : name(name), color(color), collapsed(false) {
}

void TabGroup::addTab(int tabId) {
    if (!hasTab(tabId)) {
        tabIds.push_back(tabId);
    }
}

void TabGroup::removeTab(int tabId) {
    auto it = std::find(tabIds.begin(), tabIds.end(), tabId);
    if (it != tabIds.end()) {
        tabIds.erase(it);
    }
}

bool TabGroup::hasTab(int tabId) const {
    return std::find(tabIds.begin(), tabIds.end(), tabId) != tabIds.end();
}
