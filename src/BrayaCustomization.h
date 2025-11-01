#ifndef BRAYA_CUSTOMIZATION_H
#define BRAYA_CUSTOMIZATION_H

#include <gtk/gtk.h>
#include <string>
#include <map>

class BrayaCustomization {
public:
    // Color customization structure
    struct Colors {
        // Sidebar
        std::string sidebarBg = "#0f1419";
        std::string sidebarBorder = "rgba(0, 217, 255, 0.15)";
        std::string sidebarText = "#e0e6ed";
        std::string sidebarHover = "rgba(0, 217, 255, 0.2)";
        
        // Tab bar
        std::string tabBarBg = "#0f1419";
        std::string tabActive = "#00d9ff";
        std::string tabInactive = "#6b7280";
        std::string tabHover = "rgba(0, 217, 255, 0.3)";
        std::string tabBorder = "rgba(0, 217, 255, 0.2)";
        
        // URL bar
        std::string urlBarBg = "rgba(0, 0, 0, 0.3)";
        std::string urlBarText = "#e0e6ed";
        std::string urlBarBorder = "rgba(0, 217, 255, 0.25)";
        std::string urlBarFocus = "#00d9ff";
        std::string urlBarPlaceholder = "#6b7280";
        
        // Navigation
        std::string navBtnBg = "rgba(0, 217, 255, 0.1)";
        std::string navBtnHover = "rgba(0, 217, 255, 0.25)";
        std::string navBtnDisabled = "rgba(255, 255, 255, 0.1)";
        std::string navBtnText = "#00d9ff";
        
        // Accents
        std::string accentPrimary = "#00d9ff";
        std::string accentSecondary = "#0ea5e9";
        
        // Window
        std::string windowBg = "#0a0f14";
        std::string contentBg = "#0a0f14";
        
        // Bookmarks bar
        std::string bookmarksBg = "rgba(0, 0, 0, 0.2)";
        std::string bookmarkHover = "rgba(0, 217, 255, 0.2)";
        
        // Find bar
        std::string findBarBg = "rgba(0, 0, 0, 0.4)";
        std::string findMatchBg = "#00d9ff";
        
        // Scrollbar
        std::string scrollbarTrack = "#1a1f25";
        std::string scrollbarThumb = "#374151";
        std::string scrollbarHover = "#4b5563";
    };
    
    // Typography structure
    struct Typography {
        std::string uiFont = "Sans";
        std::string urlFont = "Monospace";
        std::string tabFont = "Sans";
        
        int uiFontSize = 13;
        int urlFontSize = 14;
        int tabFontSize = 12;
        int buttonFontSize = 14;
        
        std::string uiFontWeight = "normal";
        std::string urlFontWeight = "normal";
        std::string tabFontWeight = "600";
        
        double lineHeight = 1.5;
        double letterSpacing = 0.0;
    };
    
    // Layout structure
    struct Layout {
        int sidebarWidth = 56;
        int tabHeight = 48;
        int tabWidth = 48;
        int urlBarHeight = 40;
        
        int sidebarPadding = 4;
        int tabPadding = 12;
        int urlBarPadding = 10;
        int buttonPadding = 8;
        
        int borderRadius = 8;
        int tabBorderRadius = 10;
        int urlBarBorderRadius = 24;
        int buttonBorderRadius = 8;
        
        int iconSize = 20;
        int tabIconSize = 20;
        int buttonIconSize = 16;
        
        int spacing = 4;
        int margin = 6;
    };
    
    // Effects structure
    struct Effects {
        int shadowIntensity = 12; // px
        double shadowOpacity = 0.4;
        int blurRadius = 0;
        double transparency = 1.0; // 0.0-1.0
        int animationSpeed = 200; // ms
        std::string transitionType = "ease";
        bool enableGlow = true;
        int glowIntensity = 15;
    };
    
    BrayaCustomization();
    
    void show(GtkWindow* parent);
    void applyCustomization();
    std::string generateCustomCSS();
    
    // Getters
    Colors getColors() const { return colors; }
    Typography getTypography() const { return typography; }
    Layout getLayout() const { return layout; }
    Effects getEffects() const { return effects; }
    
    // Setters
    void setColors(const Colors& c) { colors = c; }
    void setTypography(const Typography& t) { typography = t; }
    void setLayout(const Layout& l) { layout = l; }
    void setEffects(const Effects& e) { effects = e; }
    
    void save();
    void load();
    
private:
    Colors colors;
    Typography typography;
    Layout layout;
    Effects effects;
    
    GtkWidget* dialog;
    GtkWidget* notebook;
    
    void createDialog(GtkWindow* parent);
    GtkWidget* createColorsTab();
    GtkWidget* createTypographyTab();
    GtkWidget* createLayoutTab();
    GtkWidget* createEffectsTab();
    
    void saveToFile();
    void loadFromFile();
    
    // Color picker helpers
    GtkWidget* createColorPicker(const std::string& label, std::string* colorVar);
    
    static void onColorChanged(GtkColorButton* button, gpointer data);
    static void onApplyClicked(GtkButton* button, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    static void onResetClicked(GtkButton* button, gpointer data);
};

#endif
