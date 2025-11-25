# Gecko Embedding Research - Reality Check

**Date:** November 25, 2025
**Researcher:** Initial investigation into Gecko/Firefox embedding

---

## TL;DR - The Hard Truth

**libxul embedding is officially dead.** Mozilla deprecated it years ago and there is **no currently maintained desktop Gecko embedding solution**.

---

## What We Found

### 1. libxul/XULRunner Status ❌

**Official Status:** Deprecated since 2015

From Mozilla's archived documentation:
- [Gecko Embedding Basics](https://www-archive.mozilla.org/projects/embedding/embedoverview/EmbeddingGecko.pdf) (archived)
- [Embedding Overview](https://www-archive.mozilla.org/projects/embedding/embeddingoverview) (archived)
- No active maintenance
- No recent documentation (latest from 2016)

**Why it's dead:**
- Mozilla killed XULRunner project
- Deprecated embedding APIs
- Focus shifted entirely to GeckoView (Android)

**Finding:** [How to install libxul-embedding?](https://askubuntu.com/questions/378427/how-to-install-libxul-embedding) shows it's been gone for years

---

### 2. GeckoView Status ⚠️

**Official Status:** Android-only, actively maintained

From Mozilla documentation:
- [GeckoView GitHub](https://github.com/mozilla/geckoview) - "GeckoView is a set of components for embedding Gecko in **Android apps**"
- [GeckoView Docs](https://mozilla.github.io/geckoview/) - Android-focused
- [GeckoView Quick Start](https://firefox-source-docs.mozilla.org/mobile/android/geckoview/consumer/geckoview-quick-start.html) - Android only

**Desktop Linux:** Not supported

From community:
- [Hacker News discussion](https://news.ycombinator.com/item?id=29903279): "While embedding Gecko in desktop applications is not as straightforward..."
- [State of Embedding in Gecko (2016)](https://www.chrislord.net/2016/03/08/state-of-embedding-in-gecko/): No desktop solution

**Why it's Android-only:**
- Mozilla's focus is mobile
- No resources for desktop embedding
- No plans to support desktop

---

### 3. Pale Moon / Goanna 🤔

**What it is:**
- Pale Moon forked Firefox/Gecko
- Created **Goanna** engine (Gecko fork)
- Builds complete browser, not embedding

From research:
- [Pale Moon Wikipedia](https://en.wikipedia.org/wiki/Pale_Moon) - Uses Goanna (not Gecko)
- [Pale Moon Project](https://www.palemoon.org/) - Full browser, no embedding API
- [GitHub repos](https://github.com/FilipinOTech/Pale-Moon) - Browser code, not library

**Reality:** They don't embed Gecko, they forked and maintain the entire engine. Massive undertaking.

---

### 4. Current Gecko Embedding Reality

From Mozilla documentation:
- [Gecko Docs](https://firefox-source-docs.mozilla.org/overview/gecko.html): "Mozilla officially supports Gecko's use on Android, Linux, macOS, and Windows"
- **BUT** this means Firefox browser, not embeddable component
- No maintained embedding API for desktop

**What exists:**
- nsIWebBrowser interfaces (old, unsupported)
- XPCOM components (complex, deprecated)
- No GTK integration
- No documentation
- No examples since 2010s

---

## What This Means for Braya Browser

### Option 1: Try libxul Anyway (High Risk) ⚠️⚠️⚠️

**Attempt to use deprecated libxul:**

**Pros:**
- libxul.so still ships with Firefox
- Technically might work
- Could reverse-engineer from old docs

**Cons:**
- ❌ No documentation
- ❌ No support
- ❌ APIs could break anytime with Firefox update
- ❌ Mozilla could remove libxul entirely
- ❌ Would be maintaining dead code
- ❌ No community help
- ❌ Unknown bugs, no fixes

**Estimated effort:** 6-12 months (pioneering dead tech)

**Risk:** Very high - building on deprecated, unsupported foundation

---

### Option 2: Port GeckoView to Desktop (Extremely High Risk) ⚠️⚠️⚠️⚠️

**Attempt to port Android GeckoView to Linux desktop:**

**Pros:**
- Modern, maintained codebase
- Active development
- Good architecture

**Cons:**
- ❌ Android-specific code throughout
- ❌ Would need to maintain fork
- ❌ Massive engineering effort
- ❌ No community doing this
- ❌ Would lag behind official GeckoView

**Estimated effort:** 12-24 months (full-time work)

**Risk:** Extremely high - maintaining entire engine fork

---

### Option 3: Chromium/CEF (Proven, Supported) ✅

**Use Chromium Embedded Framework:**

**Pros:**
- ✅ Officially supported for desktop
- ✅ Excellent documentation
- ✅ Large community
- ✅ Known to work (Brave, Vivaldi, etc.)
- ✅ Active development
- ✅ GTK examples exist
- ✅ Will work on Windows too

**Cons:**
- ⚠️ It's Google/Chrome (not Firefox)
- ⚠️ Larger binary size
- ⚠️ Higher memory usage

**Estimated effort:** 3-4 months (well-documented path)

**Risk:** Low - proven technology, active support

**Resources:**
- [CEF Project](https://bitbucket.org/chromiumembedded/cef)
- [CEF GTK Example](https://bitbucket.org/chromiumembedded/cef/src/master/tests/cefclient/)

---

### Option 4: Stay with WebKit (Known Quantity) ✅

**Keep using WebKit2GTK:**

**Pros:**
- ✅ Already working
- ✅ Well-documented
- ✅ Active GTK integration
- ✅ Your code already written
- ✅ Known limitations

**Cons:**
- ⚠️ Performance limitations (you know these)
- ⚠️ Extension support limited
- ⚠️ Will never be as fast as Chromium

**Estimated effort:** 0 months (already done)

**Risk:** None - stick with what works

---

## Reality Check: What Can We Actually Do?

### Realistic Options (Ranked by Feasibility)

**1. Chromium/CEF** (Best option if willing to use Chrome)
- Timeline: 3-4 months
- Risk: Low
- Community: Large
- Support: Excellent
- Performance: Best
- Windows: Yes

**2. Stay WebKit** (Safest option)
- Timeline: 0 months (done)
- Risk: None
- Community: Medium
- Support: Good
- Performance: Acceptable
- Windows: No

**3. Try libxul** (Risky, unsupported)
- Timeline: 6-12 months (guessing)
- Risk: Very high
- Community: None
- Support: None
- Performance: Unknown
- Windows: Unknown

**4. Port GeckoView** (Not realistic for solo developer)
- Timeline: 12-24 months
- Risk: Extremely high
- Community: None
- Support: None
- Performance: Good (if you succeed)
- Windows: Maybe

---

## Recommendation

### We Need to Make a Choice:

**A) Go with CEF (Chromium)**
- Accept it's Chrome not Firefox
- Get proven, supported embedding
- Timeline: 3-4 months
- Can actually ship v2.0

**B) Stay with WebKit**
- Accept performance limitations
- Focus on being best WebKit browser
- Timeline: Immediate (polish current version)
- Ship v1.1 improvements

**C) Attempt libxul (High risk)**
- Accept no support, could break anytime
- Pioneering dead technology
- Timeline: 6-12 months minimum
- Might fail completely

**D) Give up on engine switch**
- Keep WebKit
- Target niche users
- Polish what we have
- Realistic about limitations

---

## My Honest Assessment

**Gecko embedding for desktop is dead.** Mozilla killed it and has no plans to revive it.

Your choices are:

1. **Use Chromium** (best performance, proven path)
2. **Stay with WebKit** (known limitations, but works)
3. **Try deprecated libxul** (high risk, might fail)

**I cannot recommend option 3** - building on officially deprecated, unsupported technology is asking for trouble.

---

## Questions for You

1. **Are you willing to use Chromium instead of Gecko?**
   - If yes: CEF is the way forward
   - If no: Stay with WebKit or risk libxul

2. **How important is "Firefox not Chrome"?**
   - Philosophical (user won't notice): CEF is fine
   - Critical requirement: Stuck with WebKit or risky libxul

3. **What's more important: ideology or functionality?**
   - Functionality: Use CEF (Chromium)
   - Ideology: Stay WebKit or attempt libxul gamble

4. **Can you accept WebKit's limitations?**
   - If yes: Polish WebKit version, ship v1.1
   - If no: Must use CEF (only proven alternative)

---

## Sources

- [Gecko Embedding Basics (Archived)](https://www-archive.mozilla.org/projects/embedding/embedoverview/EmbeddingGecko.pdf)
- [Gecko Embedding Overview (Archived)](https://www-archive.mozilla.org/projects/embedding/embeddingoverview)
- [GeckoView GitHub](https://github.com/mozilla/geckoview)
- [State of Embedding in Gecko (2016)](https://www.chrislord.net/2016/03/08/state-of-embedding-in-gecko/)
- [Pale Moon Wikipedia](https://en.wikipedia.org/wiki/Pale_Moon)
- [Stack Overflow: Embedding Gecko](https://stackoverflow.com/questions/40430903/firefox-gecko-embedded-browser)

---

**Bottom line: We found out Gecko embedding is dead. What do you want to do?**
