#ifndef TAB_GROUP_H
#define TAB_GROUP_H

#include <string>
#include <vector>
#include <gtk/gtk.h>

class TabGroup {
public:
    TabGroup(const std::string& name, const std::string& color);
    
    std::string getName() const { return name; }
    std::string getColor() const { return color; }
    bool isCollapsed() const { return collapsed; }
    std::vector<int> getTabs() const { return tabIds; }
    
    void setName(const std::string& n) { name = n; }
    void setColor(const std::string& c) { color = c; }
    void setCollapsed(bool c) { collapsed = c; }
    
    void addTab(int tabId);
    void removeTab(int tabId);
    bool hasTab(int tabId) const;
    
private:
    std::string name;
    std::string color;
    bool collapsed;
    std::vector<int> tabIds;
};

#endif
