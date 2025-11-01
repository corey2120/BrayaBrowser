# WebKit2GTK 2.50 Download Fix

The NEW way for WebKit2GTK 6.0:
- Use decide-policy to intercept downloads
- When download is triggered, get the WebKitDownload from the response
- Track it via WebKitNetworkSession (new in 6.0) or WebContext

Let me implement the proper approach.
