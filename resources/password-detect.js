// Password form detection and capture script
(function() {
    'use strict';
    
    // Find password fields on the page
    function findPasswordFields() {
        const passwordFields = document.querySelectorAll('input[type="password"]');
        return Array.from(passwordFields);
    }
    
    // Find username field near password field
    function findUsernameField(passwordField) {
        const form = passwordField.form;
        if (!form) return null;
        
        // Look for email or text input before password
        const inputs = form.querySelectorAll('input[type="email"], input[type="text"], input[type="tel"]');
        for (let input of inputs) {
            if (input.compareDocumentPosition(passwordField) & Node.DOCUMENT_POSITION_FOLLOWING) {
                return input;
            }
        }
        return null;
    }
    
    // Capture form submission
    function setupFormCapture() {
        const passwordFields = findPasswordFields();
        
        passwordFields.forEach(passwordField => {
            const form = passwordField.form;
            if (!form) return;
            
            form.addEventListener('submit', function(e) {
                const usernameField = findUsernameField(passwordField);
                const username = usernameField ? usernameField.value : '';
                const password = passwordField.value;
                
                if (username && password) {
                    // Send to browser
                    window.webkit.messageHandlers.passwordCapture.postMessage({
                        url: window.location.href,
                        username: username,
                        password: password
                    });
                }
            });
        });
    }
    
    // Auto-fill saved passwords
    window.fillPassword = function(username, password) {
        const passwordFields = findPasswordFields();
        if (passwordFields.length === 0) return;
        
        const passwordField = passwordFields[0];
        const usernameField = findUsernameField(passwordField);
        
        if (usernameField) {
            usernameField.value = username;
            usernameField.dispatchEvent(new Event('input', { bubbles: true }));
            usernameField.dispatchEvent(new Event('change', { bubbles: true }));
        }
        
        passwordField.value = password;
        passwordField.dispatchEvent(new Event('input', { bubbles: true }));
        passwordField.dispatchEvent(new Event('change', { bubbles: true }));
    };
    
    // Initialize on page load
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', setupFormCapture);
    } else {
        setupFormCapture();
    }
    
    // Also handle dynamically added forms
    const observer = new MutationObserver(setupFormCapture);
    observer.observe(document.body, { childList: true, subtree: true });
})();
