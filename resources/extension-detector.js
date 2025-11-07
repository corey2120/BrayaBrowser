// Extension detector script - injected into addons.mozilla.org and Chrome Web Store
// Detects "Add to Firefox" / "Add to Chrome" buttons and intercepts clicks

(function() {
    console.log('🔍 Braya Extension Detector loaded');

    // Check if WebKit message handler is available
    if (!window.webkit || !window.webkit.messageHandlers || !window.webkit.messageHandlers.installExtension) {
        console.error('❌ WebKit message handler not available! Cannot install extensions.');
        return;
    }

    console.log('✓ WebKit message handler available');

    // Detect if we're on an extension page
    const isFirefoxAddons = window.location.hostname.includes('addons.mozilla.org');
    const isChromeWebStore = window.location.hostname.includes('chrome.google.com') &&
                             window.location.pathname.includes('/webstore/');

    if (!isFirefoxAddons && !isChromeWebStore) {
        console.log('Not on extension page, skipping detector');
        return;
    }

    console.log('📦 Extension page detected:', isFirefoxAddons ? 'Firefox Add-ons' : 'Chrome Web Store');

    // Firefox Add-ons detection
    if (isFirefoxAddons) {
        console.log('🔎 Looking for Firefox Add-ons install button...');

        // Look for the "Add to Firefox" button
        const checkForButton = () => {
            // Try multiple selectors for Firefox Add-ons (the site structure changes over time)
            const selectors = [
                'button.InstallButton-button',
                'a.InstallButton-button',
                'button[type="button"]',
                '.InstallButtonWrapper button',
                'a[href*=".xpi"]'
            ];

            let addButton = null;
            for (const selector of selectors) {
                addButton = document.querySelector(selector);
                if (addButton) {
                    console.log(`✓ Found button with selector: ${selector}`);
                    break;
                }
            }

            if (addButton && !addButton.dataset.brayaIntercepted) {
                addButton.dataset.brayaIntercepted = 'true';

                console.log('✓ Found "Add to Firefox" button:', addButton);

                // Get the .xpi download URL - try multiple ways
                let downloadUrl = addButton.href ||
                                 addButton.getAttribute('href') ||
                                 addButton.getAttribute('data-download-url') ||
                                 addButton.getAttribute('data-href');

                // If no direct URL, look for download link in page
                if (!downloadUrl) {
                    const downloadLink = document.querySelector('a[href*=".xpi"]');
                    if (downloadLink) {
                        downloadUrl = downloadLink.href;
                        console.log('✓ Found download URL from page:', downloadUrl);
                    }
                }

                if (downloadUrl) {
                    // Change button text
                    const originalText = addButton.textContent;
                    addButton.textContent = 'Add to Braya Browser';
                    addButton.title = 'Install this extension in Braya Browser';
                    console.log('✓ Button text changed to "Add to Braya Browser"');

                    // Intercept click
                    addButton.addEventListener('click', (e) => {
                        e.preventDefault();
                        e.stopPropagation();

                        console.log('🔌 User clicked install button');
                        console.log('   Extension URL:', window.location.href);
                        console.log('   Download URL:', downloadUrl);

                        // Notify the browser to install
                        try {
                            window.webkit.messageHandlers.installExtension.postMessage({
                                url: window.location.href,
                                downloadUrl: downloadUrl
                            });
                            console.log('✓ Message sent to browser');
                        } catch (error) {
                            console.error('❌ Error sending message:', error);
                        }
                    }, true);
                } else {
                    console.warn('⚠️  Button found but no download URL');
                }
            } else if (!addButton) {
                // Log what buttons ARE on the page for debugging
                const allButtons = document.querySelectorAll('button, a');
                console.log(`No install button found yet. Page has ${allButtons.length} buttons/links`);
            }
        };

        // Check periodically for the button (it might load dynamically)
        checkForButton();
        const intervalId = setInterval(checkForButton, 1000);

        // Stop checking after 30 seconds
        setTimeout(() => clearInterval(intervalId), 30000);
    }

    // Chrome Web Store detection
    if (isChromeWebStore) {
        const checkForButton = () => {
            const addButton = document.querySelector('div[role="button"][aria-label*="Add to"]');
            if (addButton && !addButton.dataset.brayaIntercepted) {
                addButton.dataset.brayaIntercepted = 'true';

                console.log('✓ Found "Add to Chrome" button');

                // Change button text
                const textElement = addButton.querySelector('div');
                if (textElement) {
                    textElement.textContent = 'Add to Braya';
                    addButton.setAttribute('aria-label', 'Add to Braya Browser');
                }

                // Intercept click
                addButton.addEventListener('click', (e) => {
                    e.preventDefault();
                    e.stopPropagation();

                    console.log('🔌 User clicked install button');
                    console.log('   Extension URL:', window.location.href);

                    // Notify the browser to install
                    window.webkit.messageHandlers.installExtension.postMessage({
                        url: window.location.href
                    });
                }, true);
            }
        };

        checkForButton();
        setInterval(checkForButton, 1000);
    }

    console.log('✓ Extension detector initialized');
})();
