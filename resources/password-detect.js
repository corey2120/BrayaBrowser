// Enhanced password field detection with Safari-like heuristics
(function() {
    'use strict';

    let stagedUsername = null;
    let stagedUrl = null;
    const multiStepEnabled = !(window.BrayaPasswordConfig && window.BrayaPasswordConfig.multiStep === false);

    // Helper: Find username field using multiple heuristics
    function findUsernameField(context = document) {
        // Try autocomplete attribute first (Safari standard)
        let field = context.querySelector('input[autocomplete="username"], input[autocomplete="email"]');
        if (field) return field;

        // Try common name/id patterns
        const patterns = [
            'input[name*="user" i]:not([type="password"])',
            'input[name*="email" i]',
            'input[name*="login" i]:not([type="password"])',
            'input[id*="user" i]:not([type="password"])',
            'input[id*="email" i]',
            'input[id*="login" i]:not([type="password"])',
            'input[type="email"]',
            'input[type="text"][name*="account" i]'
        ];

        for (const pattern of patterns) {
            field = context.querySelector(pattern);
            if (field && isVisibleField(field)) return field;
        }

        // Fallback: first visible text/email field before password
        const passwordField = context.querySelector('input[type="password"]');
        if (passwordField) {
            const allInputs = Array.from(context.querySelectorAll('input[type="text"], input[type="email"], input:not([type])'));
            for (const input of allInputs) {
                if (isVisibleField(input) && input.compareDocumentPosition(passwordField) & Node.DOCUMENT_POSITION_FOLLOWING) {
                    return input;
                }
            }
        }

        return null;
    }

    // Helper: Find password field
    function findPasswordField(context = document) {
        // Try autocomplete attribute
        let field = context.querySelector('input[autocomplete="current-password"], input[autocomplete="new-password"]');
        if (field) return field;

        // Try type=password
        const passwordFields = Array.from(context.querySelectorAll('input[type="password"]'));
        return passwordFields.find(f => isVisibleField(f)) || passwordFields[0] || null;
    }

    // Helper: Check if field is visible
    function isVisibleField(element) {
        if (!element) return false;
        const style = window.getComputedStyle(element);
        return style.display !== 'none' && 
               style.visibility !== 'hidden' && 
               style.opacity !== '0' &&
               element.offsetWidth > 0 && 
               element.offsetHeight > 0;
    }

    // Helper: Get form fields
    function getLoginFields(form = null) {
        const context = form || document;
        return {
            username: findUsernameField(context),
            password: findPasswordField(context)
        };
    }

    // Track form submissions (including AJAX)
    const submittedForms = new WeakSet();

    // Handle traditional form submission
    function setupFormCapture() {
        document.addEventListener('submit', (e) => {
            const form = e.target;
            if (submittedForms.has(form)) return;
            submittedForms.add(form);

            const fields = getLoginFields(form);
            if (multiStepEnabled && fields.username && (!fields.password || !fields.password.value)) {
                stagedUsername = fields.username.value.trim();
                stagedUrl = window.location.href;
                return;
            }

            if (fields.username && fields.password && fields.username.value && fields.password.value) {
                // Delay capture to allow form submission to complete
                setTimeout(() => {
                    capturePassword(fields.username.value, fields.password.value);
                }, 100);
            }
        }, true);
    }

    // Handle AJAX/fetch submissions (SPA support)
    function setupAjaxCapture() {
        // Monitor password field changes
        let lastUsername = '';
        let lastPassword = '';

        document.addEventListener('input', (e) => {
            if (e.target.type === 'password' || e.target.autocomplete === 'current-password') {
                const fields = getLoginFields();
                if (fields.username && fields.password) {
                    lastUsername = fields.username.value;
                    lastPassword = fields.password.value;
                }
            }
        }, true);

        // Intercept fetch/XHR
        const originalFetch = window.fetch;
        window.fetch = function(...args) {
            return originalFetch.apply(this, args).then(response => {
                if (response.ok && lastPassword) {
                    setTimeout(() => {
                        capturePassword(lastUsername, lastPassword);
                        lastUsername = '';
                        lastPassword = '';
                    }, 500);
                }
                return response;
            });
        };
    }

    // Capture password credentials
    function capturePassword(username, password) {
        if (!password) return;
        let trimmedUser = username ? username.trim() : '';
        if (!trimmedUser && stagedUsername) {
            trimmedUser = stagedUsername;
        }
        if (!trimmedUser) {
            trimmedUser = username;
        }
        
        try {
            window.webkit.messageHandlers.passwordCapture.postMessage({
                url: stagedUrl || window.location.href,
                username: trimmedUser || '',
                password: password
            });
            stagedUsername = null;
            stagedUrl = null;
        } catch (e) {
            console.error('Failed to capture password:', e);
        }
    }

    // Autofill function - improved with field detection
    window.fillPassword = function(username, password) {
        const fields = getLoginFields();
        
        if (fields.username && fields.password) {
            // Set values
            fields.username.value = username;
            fields.password.value = password;

            // Trigger input events for frameworks like React/Vue
            ['input', 'change'].forEach(eventType => {
                fields.username.dispatchEvent(new Event(eventType, { bubbles: true }));
                fields.password.dispatchEvent(new Event(eventType, { bubbles: true }));
            });

            console.log('✓ Password autofilled');
            return true;
        }
        
        console.warn('Could not find login fields for autofill');
        return false;
    };

    // Helper: Calculate absolute coordinates for nested iframes
    function getAbsoluteRect(element) {
        const rect = element.getBoundingClientRect();
        let absoluteX = rect.left;
        let absoluteY = rect.top;

        // Walk up the frame tree to accumulate offsets
        let currentWindow = window;
        while (currentWindow !== currentWindow.parent) {
            try {
                const frameElement = currentWindow.frameElement;
                if (frameElement) {
                    const frameRect = frameElement.getBoundingClientRect();
                    absoluteX += frameRect.left;
                    absoluteY += frameRect.top;
                }
                currentWindow = currentWindow.parent;
            } catch (e) {
                // Cross-origin frame - can't traverse further
                break;
            }
        }

        return {
            x: absoluteX,
            y: absoluteY,
            width: rect.width,
            height: rect.height
        };
    }

    // Request autofill suggestions on field focus (Safari-like)
    window.requestAutofillSuggestions = function() {
        const fields = getLoginFields();
        const anchor = fields.username || fields.password;
        if (!anchor) return;

        const rect = getAbsoluteRect(anchor);

        try {
            window.webkit.messageHandlers.autofillRequest.postMessage({
                url: window.location.href,
                rect: rect
            });
        } catch (e) {
            // Handler not registered yet
        }
    };

    // Add visual key icon to fields with saved passwords
    window.addPasswordIndicators = function() {
        const fields = getLoginFields();
        
        // Add key icon indicator to fields
        if (fields.username || fields.password) {
            const style = document.createElement('style');
            style.id = 'braya-password-style';
            style.textContent = `
                input[data-braya-has-password] {
                    background-image: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16" fill="%23666"><path d="M8 1a3 3 0 0 0-3 3v2H3.5A1.5 1.5 0 0 0 2 7.5v6A1.5 1.5 0 0 0 3.5 15h9a1.5 1.5 0 0 0 1.5-1.5v-6A1.5 1.5 0 0 0 12.5 6H11V4a3 3 0 0 0-3-3zm1 6h1.5a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-.5.5h-9A.5.5 0 0 1 2 13.5v-6a.5.5 0 0 1 .5-.5H9z"/></svg>');
                    background-repeat: no-repeat;
                    background-position: right 8px center;
                    background-size: 16px 16px;
                    padding-right: 30px !important;
                    cursor: pointer;
                }
                input[data-braya-has-password]:hover {
                    background-color: #f0f8ff;
                }
            `;
            
            // Remove old style if exists
            const oldStyle = document.getElementById('braya-password-style');
            if (oldStyle) oldStyle.remove();
            
            document.head.appendChild(style);
            
            // Mark fields
            [fields.username, fields.password].forEach(field => {
                if (!field) return;
                field.setAttribute('data-braya-has-password', 'true');
                if (!field.dataset.brayaIndicatorBound) {
                    field.addEventListener('focus', requestAutofillSuggestions, { capture: true });
                    field.addEventListener('click', requestAutofillSuggestions, { capture: true });
                    field.dataset.brayaIndicatorBound = 'true';
                }
            });
            
            console.log('✓ Added password indicators');
        }
    };

    // Check if passwords available for this URL
    window.checkPasswordsAvailable = function() {
        try {
            window.webkit.messageHandlers.checkPasswords.postMessage({
                url: window.location.href
            });
        } catch (e) {
            // Handler not registered
        }
    };

    // Add focus listeners for autofill triggers
    function setupFocusListeners() {
        document.addEventListener('focusin', (e) => {
            const target = e.target;
            if (target.tagName === 'INPUT') {
                const fields = getLoginFields();
                if (target === fields.username || target === fields.password) {
                    requestAutofillSuggestions();
                }
            }
        }, true);
    }

    // Initialize when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

    function init() {
        setupFormCapture();
        setupAjaxCapture();
        setupFocusListeners();
        
        // Check if passwords are available and add visual indicators
        checkPasswordsAvailable();
        
        console.log('✓ Braya password manager initialized');
    }

    // Re-initialize on dynamic content changes (SPAs)
    const observer = new MutationObserver(() => {
        const fields = getLoginFields();
        if (fields.password) {
            requestAutofillSuggestions();
            checkPasswordsAvailable();
            addPasswordIndicators();
        }
    });

    observer.observe(document.body, {
        childList: true,
        subtree: true
    });
})();
