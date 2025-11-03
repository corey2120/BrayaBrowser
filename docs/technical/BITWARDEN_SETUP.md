# Bitwarden Integration Setup Guide

## Overview
Braya Browser v1.0.1-beta2 includes full Bitwarden integration for password sync and management.

## Prerequisites

### 1. Install Bitwarden CLI
```bash
# Option 1: Using npm (recommended)
npm install -g @bitwarden/cli

# Option 2: Download from official site
# Visit: https://bitwarden.com/download/
```

### 2. Install jq (JSON processor)
```bash
sudo dnf install jq
```

### 3. Verify Installation
```bash
# Check Bitwarden CLI
bw --version
# Should show: 2024.x.x or similar

# Check jq
jq --version
# Should show: jq-1.x or similar
```

## Setup Instructions

### Step 1: Login to Bitwarden
```bash
bw login
```

You'll be prompted for:
- Email address
- Master password
- (Optional) Two-factor authentication code

### Step 2: Unlock Your Vault
```bash
bw unlock
```

This will provide a session key that lasts for a limited time.

## Using Bitwarden in Braya Browser

### Method 1: Sync from Bitwarden (Recommended)
This imports all your login items from Bitwarden and keeps them in sync.

1. Open Braya Browser
2. Click the 🔑 **Password Manager** button (or press Ctrl+K)
3. Click **🔐 Bitwarden**
4. Select **🔄 Sync from Bitwarden**
5. Wait for sync to complete
6. Success dialog will appear when done

**What happens:**
- Downloads all login items from Bitwarden vault
- Imports them into Braya's password manager
- Existing passwords are updated if they match
- New passwords are added

### Method 2: One-Time Import
Same as sync, but doesn't update Bitwarden.

1. Open Password Manager
2. Click **🔐 Bitwarden**
3. Select **📥 Import from Bitwarden**

### Method 3: Export to Bitwarden
Export Braya passwords to Bitwarden format for manual import.

1. Open Password Manager
2. Click **🔐 Bitwarden**
3. Select **📤 Export to Bitwarden**
4. CSV file created at: `~/braya-passwords-for-bitwarden.csv`
5. Import in Bitwarden web vault: **Tools > Import Data**

## Troubleshooting

### Error: "Bitwarden CLI (bw) is not installed"
**Solution:**
```bash
npm install -g @bitwarden/cli
```

### Error: "jq is not installed"
**Solution:**
```bash
sudo dnf install jq
```

### Error: "Not logged in to Bitwarden"
**Solution:**
```bash
bw login
# Enter your email and master password
```

### Error: "Failed to unlock Bitwarden vault"
**Solution:**
```bash
# Unlock manually and note the session key
bw unlock

# Or check if you need to login again
bw login
```

### Error: "No passwords found in Bitwarden vault"
**Possible causes:**
- Vault is empty
- Only secure notes/cards (not login items)
- Network connection issue

**Solution:**
- Check Bitwarden web vault has login items
- Try: `bw list items` to see what's there

### Session Timeout
Bitwarden sessions timeout after 15-30 minutes for security.

**Solution:**
- Unlock vault again: `bw unlock`
- Or keep session alive with periodic syncs

## How It Works (Technical)

### Import Process:
1. Checks if `bw` CLI is installed
2. Checks if `jq` is installed
3. Verifies user is logged in: `bw status`
4. Unlocks vault and gets session key: `bw unlock --raw`
5. Lists all login items: `bw list items --session <key>`
6. Filters for login type (type == 1)
7. Converts to CSV format using jq
8. Imports CSV into Braya password manager
9. Displays success/error dialog

### Data Extracted:
- **URL:** From login.uris[0].uri or item.name
- **Username:** From login.username
- **Password:** From login.password

### Security:
- Session keys are temporary files (deleted immediately)
- CSV exports are in `~/.cache/braya-browser/` (0700 permissions)
- Temporary files are cleaned up after import
- No passwords stored in plain text on disk (Braya uses XOR encryption)

## Alternative: Manual CSV Import

If Bitwarden CLI doesn't work for you:

### From Bitwarden Web Vault:
1. Login to https://vault.bitwarden.com
2. Go to **Tools > Export Vault**
3. Format: **CSV**
4. Download the file
5. In Braya: Password Manager > **📥 Import**
6. Select the downloaded CSV file

### CSV Format:
Braya expects CSV with these columns (in order):
```
url,username,password
```

Example:
```csv
url,username,password
https://github.com,user@email.com,mypassword123
https://reddit.com,myusername,anotherpass
example.com,admin,securepass
```

## Best Practices

### Keep Passwords in Sync:
1. Use Bitwarden as your primary password manager
2. Sync to Braya when you add new passwords
3. Or use Braya's auto-save and periodically export to Bitwarden

### Security:
1. Use strong master password in Bitwarden
2. Enable two-factor authentication
3. Lock Bitwarden vault when not in use
4. Don't share session keys
5. Regularly update Braya's saved passwords

### Backup:
1. Export passwords from both Braya and Bitwarden
2. Store encrypted backups
3. Keep master password secure and backed up

## Support

If you continue to have issues:
1. Check Bitwarden CLI documentation: https://bitwarden.com/help/cli/
2. Run commands manually to diagnose:
   ```bash
   bw status
   bw list items
   ```
3. Check Braya console output for detailed errors
4. Report issues to Braya Browser project

---

**Version:** v1.0.1-beta2
**Last Updated:** November 2, 2024
