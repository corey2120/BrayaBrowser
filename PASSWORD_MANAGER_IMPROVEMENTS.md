# Password Manager Improvements Plan

## Current State Analysis

### ✅ What Works Well
- **Strong encryption**: AES-256-CBC with Argon2id key derivation
- **Master password**: Vault locking with timeout
- **Auto-save/auto-fill**: Basic form detection
- **CSV import/export**: Chrome format compatible
- **Password generator**: 16-char random with symbols
- **Strength calculator**: Basic scoring (0-100)

### ❌ What Needs Improvement
1. **UI/UX**: Basic list, no categories, no search, no favorites
2. **Security**: No breach detection, weak password warnings only basic
3. **Autofill**: Limited to single-page logins, no keyboard shortcuts
4. **Generator**: Only random strings, no passphrases or memorable passwords
5. **Import**: Only Chrome CSV, missing 1Password/Bitwarden/LastPass
6. **Organization**: No tags, categories, or folders

---

## 🎯 Implementation Plan

### **Phase A: Enhanced UI** (High Priority)

#### A1. Categories & Organization
- **Favorites system**: Star important passwords for quick access
- **Categories**: Work, Personal, Banking, Social, Shopping, Custom
- **Tags**: Multi-tag support (e.g., "work", "urgent", "shared")
- **Search**: Real-time filter by URL, username, category, tags
- **Sort options**: By name, date added, last used, strength

**UI Mockup:**
```
┌────────────────────────────────────────────────┐
│ 🔐 Password Vault          [🔍 Search...]  [+] │
├─────────────┬──────────────────────────────────┤
│ All (45) ⭐  │ GitHub - myusername              │
│ Favorites(5)│ https://github.com/login         │
│ Weak (3) ⚠️  │                                  │
│ Reused (5)  │ ⚠️ Weak password (35/100)        │
│ Breached(0) │ 🔁 Reused on 2 other sites       │
│             │                                  │
│ Categories: │ [👁 Show] [📋 Copy] [✏️ Edit]    │
│  💼 Work(12)│                                  │
│  🏠 Personal│ Category: Work                   │
│  💳 Banking │ Tags: github, development        │
│  📱 Social  │ Created: 3 months ago            │
│  🛒 Shopping│ Last used: 2 days ago            │
│  ➕ Custom  │ Strength: ████░░ 35/100          │
└─────────────┴──────────────────────────────────┘
```

#### A2. Smart Lists (Auto-Generated)
- **Favorites**: User-starred passwords
- **Weak Passwords**: Strength < 40
- **Reused Passwords**: Same password on multiple sites
- **Breached**: Found in HaveIBeenPwned database
- **Recently Added**: Last 30 days
- **Never Used**: Created but never auto-filled

#### A3. Detail Pane Improvements
- **Larger, more readable layout**
- **Action buttons**: View, Copy Username, Copy Password, Edit, Delete
- **Metadata**: Created, updated, last used timestamps
- **Usage statistics**: Times auto-filled, times copied
- **Security warnings**: Weak, reused, breached, old (90+ days)
- **Related accounts**: Show other accounts on same domain

---

### **Phase B: Security Enhancements** (High Priority)

#### B1. Password Breach Detection
**Integration with HaveIBeenPwned API:**
- Check passwords against 850M+ breached credentials
- Uses k-anonymity (only sends first 5 chars of SHA-1 hash)
- Async checking to avoid blocking UI
- Cache results locally
- Quarterly re-checks

**Implementation:**
```cpp
class PasswordBreachChecker {
    bool checkPassword(const std::string& password);
    void checkAllPasswords();  // Background scan
    std::string getHashPrefix(const std::string& password);
    bool queryPwnedAPI(const std::string& prefix);
};
```

#### B2. Enhanced Password Analysis
**Current:** Basic length + character variety (max 100)

**Improved:**
- **Dictionary words**: Penalize common words (-20 points)
- **Sequential chars**: Detect 123, abc patterns (-10 points)
- **Repeated chars**: Detect aaa, 111 patterns (-5 points)
- **Common passwords**: Check against top-10k list (-30 points)
- **Entropy calculation**: Bits of randomness
- **Pattern detection**: keyboard patterns (qwerty, asdfgh)

**Strength Labels:**
- 0-20: Very Weak (Red) ❌
- 21-40: Weak (Orange) ⚠️
- 41-60: Moderate (Yellow) 🟡
- 61-80: Strong (Light Green) ✅
- 81-100: Very Strong (Dark Green) 💪

#### B3. Password Reuse Detection
- Scan all passwords for duplicates
- Show warning: "⚠️ Reused on 3 sites: gmail.com, facebook.com, twitter.com"
- One-click "Generate Unique" for each site
- Track password history per account (last 5 passwords)

#### B4. Password Expiry Warnings
- **Old passwords**: Not changed in 90+ days
- **Never changed**: Still using created password after 6+ months
- **Post-breach**: If site was breached, prompt password change
- **Suggested action**: "Update password" button

---

### **Phase C: Smart Autofill** (Medium Priority)

#### C1. Multi-Step Login Detection
**Problem:** Sites with separate username/password pages (Microsoft, Google)

**Solution:**
- Detect when username field submitted but no password field
- Store username temporarily
- When password page loads, auto-fill password
- Match by domain + session tracking

#### C2. Keyboard Shortcuts
- `Ctrl+Shift+L`: Open autofill dropdown on focused field
- `Ctrl+Shift+G`: Generate password for current field
- `Ctrl+Shift+V`: Show vault window
- `Ctrl+Shift+C`: Copy password for current site

#### C3. Smart Field Detection
**Current:** Basic input[type=password] detection

**Improved:**
- **Detect by name attribute**: email, username, user, login, account
- **Detect by ID**: #username, #email, #password, #login
- **Detect by placeholder**: "Enter email", "Username"
- **Detect by autocomplete**: autocomplete="username"
- **Cross-frame**: Detect fields in iframes
- **OTP fields**: Detect 2FA code inputs (don't save these)

#### C4. Account Selector
When multiple accounts for same site:
```
┌─────────────────────────────┐
│ Choose account for:         │
│ github.com                  │
├─────────────────────────────┤
│ ○ alice@example.com ⭐      │
│ ○ bob@work.com              │
│ ○ charlie@personal.com      │
├─────────────────────────────┤
│ [Fill Selected] [Cancel]    │
└─────────────────────────────┘
```

---

### **Phase D: Password Generator** (Medium Priority)

#### D1. Multiple Generation Modes

**1. Random Password (Current)**
- Length: 8-64 characters
- Toggles: Uppercase, Lowercase, Numbers, Symbols
- Exclude ambiguous: l, 1, I, O, 0
- Custom charset

**2. Memorable Passphrase**
```
correct-horse-battery-staple
glowing-sunset-42-dancing
autumn-river-swift-mountain
```
- EFF wordlist (7,776 words)
- 4-6 words, separator customizable
- Optional number insertion
- Entropy: ~50-80 bits

**3. Pronounceable Password**
```
TrembiQuos8#
VolarDex42!
KimproNal9@
```
- Alternating consonant-vowel patterns
- Still includes numbers/symbols
- Easier to remember than random
- Medium security

**4. PIN Generator**
- 4-8 digit PINs
- Avoids common PINs (1234, 0000)
- For mobile apps, tablets

**UI:**
```
┌──────────────────────────────────────┐
│ Password Generator                   │
├──────────────────────────────────────┤
│ Mode: ○ Random  ●Passphrase  ○ PIN   │
├──────────────────────────────────────┤
│ Words: [4] ▼                         │
│ Separator: [-] ▼                     │
│ Include number: ☑                    │
│ Capitalize: ☑                        │
├──────────────────────────────────────┤
│ Generated:                           │
│ Sunset-Mountain-River-42             │
├──────────────────────────────────────┤
│ Strength: ████████░░ 82/100 (Strong) │
│ Entropy: 51.7 bits                   │
├──────────────────────────────────────┤
│ [🔄 Regenerate] [📋 Copy] [Use This] │
└──────────────────────────────────────┘
```

---

### **Phase E: Import/Export** (Low Priority)

#### E1. Import Formats
**Currently:** Chrome CSV only

**Add:**
1. **1Password** (.1pif JSON export)
2. **Bitwarden** (JSON/CSV export)
3. **LastPass** (CSV export)
4. **Firefox** (logins.json)
5. **KeePass** (CSV export)
6. **Dashlane** (CSV export)
7. **Generic CSV** (flexible column mapping)

**Import Wizard:**
```
Step 1: Select Format
  ○ Chrome CSV
  ○ Bitwarden JSON
  ○ LastPass CSV
  ○ 1Password
  ○ Firefox JSON
  ○ Generic CSV (map columns)

Step 2: Preview
  Found 47 passwords to import
  ☑ Skip duplicates (URL + username match)
  ○ Replace duplicates
  ○ Keep both

Step 3: Import
  [Progress bar]
  Imported 43 new passwords
  Skipped 4 duplicates
```

#### E2. Export Formats
**Currently:** Chrome CSV only

**Add:**
1. **Encrypted JSON** (with password protection)
2. **Bitwarden JSON** (for migration)
3. **Markdown** (human-readable documentation)
4. **HTML** (printable backup)
5. **KeePass CSV** (for migration)

**Export Options:**
- Export all vs. selected category
- Include/exclude metadata (dates, strength, tags)
- Password protect export file
- Scheduled automatic backups

---

### **Phase F: Additional Features** (Nice to Have)

#### F1. Secure Notes
Store non-password secrets:
- Credit card numbers (encrypted)
- Software license keys
- WiFi passwords
- Recovery codes (2FA backup)
- Passport numbers
- SSN (for taxes)

#### F2. Password Sharing
- Share password with another Braya user
- Time-limited access (expires after 24h)
- View-only vs. edit permissions
- Revoke access anytime
- Encrypted end-to-end

#### F3. Emergency Access
- Designate trusted contact
- They can request access
- You have 48h to deny
- If no response, they get access
- For estate planning, emergencies

#### F4. Vault Health Dashboard
```
┌────────────────────────────────────┐
│ 🔐 Vault Health Score: 67/100     │
├────────────────────────────────────┤
│ ⚠️ 3 weak passwords                │
│ 🔁 5 reused passwords              │
│ 🚨 0 breached passwords            │
│ ⏰ 7 passwords not changed in 90d  │
│                                    │
│ [Fix Weak] [Fix Reused] [Update]  │
└────────────────────────────────────┘
```

---

## 📅 Implementation Timeline

### Week 1-2: Enhanced UI (Phase A)
- Categories and tags system
- Favorites and smart lists
- Search and filter
- Improved detail pane

### Week 3: Security (Phase B)
- Password breach detection API
- Enhanced strength calculator
- Reuse detection
- Expiry warnings

### Week 4: Generator (Phase D)
- Passphrase mode
- Pronounceable passwords
- UI improvements

### Week 5: Autofill (Phase C)
- Multi-step login
- Keyboard shortcuts
- Smart field detection

### Week 6: Import/Export (Phase E)
- Additional import formats
- Export options
- Backup automation

---

## 🎯 Quick Wins (Start Here)

1. **Favorites system** (2 hours)
   - Add boolean flag to PasswordEntry
   - Star icon in UI
   - Favorites filter list

2. **Search/filter** (3 hours)
   - GtkSearchEntry in toolbar
   - Real-time filtering
   - Match URL, username, category

3. **Weak password warnings** (2 hours)
   - Visual indicator (⚠️) for strength < 40
   - Automatic "Weak" smart list
   - Quick "Generate Strong" button

4. **Passphrase generator** (4 hours)
   - EFF wordlist file
   - New generator mode
   - 4-word default

5. **Breach check** (6 hours)
   - HaveIBeenPwned API integration
   - Async checking
   - Cache results

**Total Quick Wins:** ~17 hours for high-impact improvements

---

## 🔧 Technical Architecture

### New Classes

```cpp
// Categories and organization
class PasswordCategory {
    std::string id;
    std::string name;
    std::string icon;
    std::string color;
};

// Enhanced password entry
struct PasswordEntry {
    // Existing fields
    std::string url;
    std::string username;
    std::string password;
    long createdAt, updatedAt, lastUsedAt;
    int strengthScore;
    bool breached;

    // New fields
    bool favorite;
    std::string category;
    std::vector<std::string> tags;
    int usageCount;
    std::vector<std::string> passwordHistory;
    long lastBreachCheck;
    std::string notes;  // Encrypted notes
};

// Breach checking
class PasswordBreachChecker {
    std::map<std::string, bool> cache;
    std::string computeSHA1(const std::string& password);
    bool checkAgainstPwnedAPI(const std::string& password);
    void updateBreachStatus(PasswordEntry& entry);
};

// Password analysis
class PasswordAnalyzer {
    int calculateEntropy(const std::string& password);
    bool containsDictionaryWords(const std::string& password);
    bool hasSequentialPatterns(const std::string& password);
    bool isCommonPassword(const std::string& password);
    std::vector<std::string> getSuggestions(const PasswordEntry& entry);
};

// Enhanced generator
class PasswordGenerator {
    enum Mode { RANDOM, PASSPHRASE, PRONOUNCEABLE, PIN };
    std::string generate(Mode mode, int length);
    std::string generatePassphrase(int words, char separator);
    std::string generatePronounceable(int length);
};
```

---

## ✅ Success Metrics

Password Manager improvements will be successful if:

1. **Usability**
   - Users can find passwords in <2 seconds
   - Categories reduce scroll time by 70%
   - Search finds passwords instantly

2. **Security**
   - 90% of passwords rated "Strong" or better
   - Zero breached passwords in use
   - Zero password reuse across critical sites

3. **Adoption**
   - Users generate strong passwords 80% of the time
   - Autofill success rate >95%
   - Import from other managers works first try

4. **Performance**
   - Vault opens in <200ms
   - Search filters in <50ms
   - Breach check completes in <2s

---

**Ready to start implementation?** Let's begin with the Quick Wins! 🚀
