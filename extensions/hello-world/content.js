// Content script for Hello World extension
// This runs in the context of web pages

console.log('🌐 Hello World Extension content script loaded on:', window.location.href);

// Add a banner to the page
const banner = document.createElement('div');
banner.textContent = '👋 Hello from Braya Browser Extension!';
banner.style.cssText = `
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 10px 20px;
    text-align: center;
    font-family: Arial, sans-serif;
    font-size: 14px;
    font-weight: bold;
    z-index: 999999;
    box-shadow: 0 2px 10px rgba(0,0,0,0.3);
`;

// Add close button
const closeBtn = document.createElement('button');
closeBtn.textContent = '×';
closeBtn.style.cssText = `
    position: absolute;
    right: 10px;
    top: 50%;
    transform: translateY(-50%);
    background: rgba(255,255,255,0.2);
    border: none;
    color: white;
    font-size: 20px;
    cursor: pointer;
    padding: 0 8px;
    border-radius: 3px;
`;
closeBtn.onclick = () => banner.remove();

banner.appendChild(closeBtn);
document.body.appendChild(banner);

// Send message to background script
chrome.runtime.sendMessage({
    type: 'content-loaded',
    url: window.location.href
}, (response) => {
    console.log('📬 Content script got response from background:', response);
});
