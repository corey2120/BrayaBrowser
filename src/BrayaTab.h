#ifndef BRAYA_TAB_H
#define BRAYA_TAB_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string>

class BrayaTab {
public:
    BrayaTab(int id, const char* url = "about:braya");
    ~BrayaTab();
    
    int getId() const { return id; }
    WebKitWebView* getWebView() { return webView; }
    GtkWidget* getScrolledWindow() { return scrolledWindow; }
    GtkWidget* getTabButton() { return tabButton; }
    
    std::string getTitle() const { return title; }
    std::string getUrl() const { return url; }
    
    void setTabButton(GtkWidget* button) { tabButton = button; }
    void updateButton();
    
private:
    int id;
    std::string title;
    std::string url;
    bool isLoading;
    GdkTexture* favicon;
    
    WebKitWebView* webView;
    GtkWidget* scrolledWindow;
    GtkWidget* tabButton;
    
    static void onLoadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer userData);
    static void onTitleChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onUriChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onFaviconChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onWebProcessCrashed(WebKitWebView* webView, gpointer userData);
};

#endif // BRAYA_TAB_H
