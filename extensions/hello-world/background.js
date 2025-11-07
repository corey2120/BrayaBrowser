// Background script for Hello World extension

console.log('🚀 Hello World Extension background script loaded!');

// Listen for messages from content scripts
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    console.log('📨 Background received message:', message);
    sendResponse({status: 'ok', from: 'background'});
});

// Log when extension is installed
chrome.runtime.onInstalled.addListener(() => {
    console.log('✅ Hello World Extension installed!');
});

console.log('Background script initialized');
