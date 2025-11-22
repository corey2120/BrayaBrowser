#ifndef BRAYA_CUSTOMIZATION_H
#define BRAYA_CUSTOMIZATION_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <map>

// Simplified theme structure focusing on CSS variables
struct ThemeColors {
    std::string name = "Custom";
    std::string accentColor = "#00d9ff";      // --braya-accent
    std::string bgPrimary = "#0a0f14";         // --braya-bg-primary
    std::string bgSecondary = "#0f1419";       // --braya-bg-secondary
    std::string bgTertiary = "#1a1f26";        // --braya-bg-tertiary
    std::string textPrimary = "#e0e6ed";       // --braya-text-primary
    std::string textSecondary = "#9ca3b0";     // --braya-text-secondary
};

class BrayaCustomization {
public:
    // Simplified color structure matching CSS variables
    struct Colors {
        std::string accentPrimary = "#00d9ff";
        std::string bgPrimary = "#0a0f14";
        std::string bgSecondary = "#0f1419";
        std::string bgTertiary = "#1a1f26";
        std::string textPrimary = "#e0e6ed";
        std::string textSecondary = "#9ca3b0";
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

    // UI Visibility - Control what's shown/hidden
    struct UIVisibility {
        bool showBookmarksBar = true;
        bool showStatusBar = false;
        bool showSidebar = true;
        bool showBackButton = true;
        bool showForwardButton = true;
        bool showReloadButton = true;
        bool showHomeButton = true;
        bool showDownloadsButton = true;
        bool showExtensionsButton = true;
        bool showSettingsButton = true;
        bool showSecurityIndicator = true;
        bool compactMode = false;
    };

    // Advanced Layout
    struct AdvancedLayout {
        // Tab settings
        int tabMinWidth = 180;
        int tabMaxWidth = 280;
        int tabHeight = 48;
        bool showTabCloseButton = true;
        bool showTabIcons = true;
        bool animateTabs = true;

        // URL bar
        int urlBarHeight = 40;
        int urlBarBorderRadius = 24;
        bool urlBarShowProtocol = false;
        bool urlBarCenterText = false;

        // Sidebar
        int sidebarWidth = 56;
        bool sidebarAutoHide = false;
        std::string sidebarPosition = "left"; // left or right

        // Spacing
        int globalSpacing = 4;
        int buttonSpacing = 8;
        int tabSpacing = 2;
    };
    
    BrayaCustomization();
    
    void show(GtkWindow* parent);
    void applyCustomization();
    std::string generateCustomCSS();

    // Theme preset methods
    void loadPreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets();
    void setAccentColor(const std::string& hexColor);

    // Apply theme to window
    void applyTheme(GtkWidget* window);

    // Getters
    Colors getColors() const { return colors; }
    Typography getTypography() const { return typography; }
    Layout getLayout() const { return layout; }
    Effects getEffects() const { return effects; }
    UIVisibility getUIVisibility() const { return uiVisibility; }
    AdvancedLayout getAdvancedLayout() const { return advancedLayout; }
    ThemeColors getCurrentTheme() const;

    // Setters
    void setColors(const Colors& c) { colors = c; }
    void setTypography(const Typography& t) { typography = t; }
    void setLayout(const Layout& l) { layout = l; }
    void setEffects(const Effects& e) { effects = e; }
    void setUIVisibility(const UIVisibility& v) { uiVisibility = v; }
    void setAdvancedLayout(const AdvancedLayout& l) { advancedLayout = l; }

    void save();
    void load();

    // Custom CSS
    void loadCustomCSS(const std::string& cssFile);
    void importCustomCSS();
    std::string getCustomCSSPath();
    
private:
    Colors colors;
    Typography typography;
    Layout layout;
    Effects effects;
    UIVisibility uiVisibility;
    AdvancedLayout advancedLayout;

    GtkWidget* dialog;
    GtkWidget* notebook;
    GtkCssProvider* cssProvider;

    // Theme presets
    std::map<std::string, ThemeColors> presets;
    void initializePresets();

    void createDialog(GtkWindow* parent);
    GtkWidget* createColorsTab();
    GtkWidget* createCustomCSSTab();
    GtkWidget* createUIVisibilityTab();
    GtkWidget* createAdvancedTab();
    GtkWidget* createTypographyTab();
    GtkWidget* createLayoutTab();
    GtkWidget* createEffectsTab();

    void saveToFile();
    void loadFromFile();
    std::string getConfigPath();
    std::string hexToRGB(const std::string& hex);

    // Color picker helpers
    GtkWidget* createColorPicker(const std::string& label, std::string* colorVar);

    static void onColorChanged(GtkColorButton* button, gpointer data);
    static void onApplyClicked(GtkButton* button, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    static void onResetClicked(GtkButton* button, gpointer data);
};

#endif
