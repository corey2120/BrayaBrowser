# 🚀 Publishing BrayaBrowser to AUR - Quick Guide

Now that your repository is **PUBLIC**, users worldwide can install BrayaBrowser via AUR!

## ⚡ Quick Publish Steps

### Step 1: Commit Your Fixes to GitHub

First, let's push the bug fixes to your public repo:

```bash
cd /home/cobrien/Documents/BrayaBrowser

# Check what needs to be committed
git status

# Add the fixed header file
git add src/BrayaTab.h

# Optional: Add AUR files to repo for reference
git add PKGBUILD .SRCINFO BUILD_INSTALL.md AUR_SETUP_COMPLETE.md

# Commit the fixes
git commit -m "Fix: Add missing member variables to BrayaTab for suspend feature"

# Push to GitHub
git push origin main
```

### Step 2: Create a Git Tag (Important!)

The PKGBUILD references `v1.0.6` tag, so create it:

```bash
# Create the tag
git tag -a v1.0.6 -m "Release version 1.0.6 - AUR ready"

# Push the tag to GitHub
git push origin v1.0.6
```

### Step 3: Set Up AUR Account

If you haven't already:

1. **Register** at https://aur.archlinux.org/register
2. **Add SSH Key**:
   ```bash
   # Generate key if needed
   ssh-keygen -t ed25519 -C "your_email@example.com"
   
   # Copy public key
   cat ~/.ssh/id_ed25519.pub
   
   # Paste at: https://aur.archlinux.org/account/
   ```

### Step 4: Publish to AUR

```bash
# Create AUR directory
mkdir -p ~/aur
cd ~/aur

# Clone the AUR repository (creates new package)
git clone ssh://aur@aur.archlinux.org/braya-browser.git
cd braya-browser

# Copy package files
cp /home/cobrien/Documents/BrayaBrowser/PKGBUILD .
cp /home/cobrien/Documents/BrayaBrowser/.SRCINFO .

# Create .gitignore
cat > .gitignore << 'EOF'
*
!PKGBUILD
!.SRCINFO
!.gitignore
EOF

# Review the files
cat PKGBUILD
cat .SRCINFO

# Commit and push to AUR
git add PKGBUILD .SRCINFO .gitignore
git commit -m "Initial import: BrayaBrowser 1.0.6 - Modern WebKit browser with extensions"
git push
```

### Step 5: Verify Your Package

Your package will be live at:
**https://aur.archlinux.org/packages/braya-browser**

## 🎉 Users Can Now Install!

Anyone can now install BrayaBrowser with:

### Using yay:
```bash
yay -S braya-browser
```

### Using paru:
```bash
paru -S braya-browser
```

### Manual installation:
```bash
git clone https://aur.archlinux.org/braya-browser.git
cd braya-browser
makepkg -si
```

## 📢 Promote Your Package

Share your AUR package:

1. **Reddit**: r/archlinux, r/linux
2. **Twitter/X**: "#ArchLinux #AUR #OpenSource"
3. **Your website**: https://braya.dev
4. **GitHub README**: Add installation instructions

Example badge for README:
```markdown
[![AUR version](https://img.shields.io/aur/version/braya-browser)](https://aur.archlinux.org/packages/braya-browser)
[![AUR votes](https://img.shields.io/aur/votes/braya-browser)](https://aur.archlinux.org/packages/braya-browser)
```

## 🔄 Updating the Package

When you release version 1.0.7:

### 1. Update your code and tag:
```bash
cd /home/cobrien/Documents/BrayaBrowser

# Make your changes...

# Commit
git add .
git commit -m "Release v1.0.7: New features..."

# Create and push tag
git tag -a v1.0.7 -m "Version 1.0.7"
git push origin main --tags
```

### 2. Update PKGBUILD:
```bash
# Edit version
sed -i 's/pkgver=1.0.6/pkgver=1.0.7/' PKGBUILD
sed -i 's/pkgrel=1/pkgrel=1/' PKGBUILD  # Reset to 1 for new version

# Regenerate .SRCINFO
makepkg --printsrcinfo > .SRCINFO
```

### 3. Push to AUR:
```bash
cd ~/aur/braya-browser
cp /home/cobrien/Documents/BrayaBrowser/PKGBUILD .
cp /home/cobrien/Documents/BrayaBrowser/.SRCINFO .

git add PKGBUILD .SRCINFO
git commit -m "Update to 1.0.7"
git push
```

## 📊 Monitor Your Package

- **Check build status**: AUR users will comment if it doesn't build
- **Respond to comments**: https://aur.archlinux.org/packages/braya-browser
- **Update regularly**: Keep your package maintained
- **Mark out of date**: Users can flag if it's outdated

## 🐛 If Users Report Build Issues

1. Test locally first:
   ```bash
   cd /home/cobrien/Documents/BrayaBrowser
   ./build-aur.sh
   ```

2. Fix the issue in your GitHub repo
3. Create a new tag if needed
4. Update AUR PKGBUILD and bump pkgrel

## 🏆 Best Practices

- **Respond quickly** to comments and issues
- **Test before publishing** updates
- **Document changes** in commit messages  
- **Keep dependencies updated** in PKGBUILD
- **Follow Arch packaging standards**

## 📝 Optional: Clean Up PKGBUILD

Since you've fixed the code in your repo, after pushing the fix you can simplify the PKGBUILD by removing the `prepare()` function. The current version works, but a cleaner version would be:

```bash
# After v1.0.7 or next release, remove the prepare() section
# since the fixes will be in the git tag
```

## ✅ Checklist

- [ ] Commit fixes to GitHub repo
- [ ] Create and push v1.0.6 tag
- [ ] Set up AUR account with SSH key
- [ ] Clone AUR repo and push PKGBUILD
- [ ] Test installation with `yay -S braya-browser`
- [ ] Update README.md with AUR installation instructions
- [ ] Share on social media/forums

---

**🎊 Congratulations! Your browser is now available to the entire Arch Linux community!**

For questions or issues, users can reach you at:
- GitHub: https://github.com/corey2120/BrayaBrowser/issues
- AUR Comments: https://aur.archlinux.org/packages/braya-browser
- Email: corey@braya.dev
