#!/bin/bash
# Install GStreamer codecs for Braya Browser video playback

set -euo pipefail

FEDORA_VERSION="$(rpm -E %fedora)"

echo "Installing RPM Fusion (needed for H.264/MP3 codecs)..."
if ! rpm -qa | grep -q rpmfusion-free-release; then
    sudo dnf install -y "https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORA_VERSION}.noarch.rpm"
fi
if ! rpm -qa | grep -q rpmfusion-nonfree-release; then
    sudo dnf install -y "https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORA_VERSION}.noarch.rpm"
fi

echo "Installing base GStreamer plugins..."
sudo dnf install -y gstreamer1-plugins-base gstreamer1-plugins-good gstreamer1-plugins-bad-free gstreamer1-plugins-ugly-free

echo "Installing libav and non-free codecs (H.264/MP3)..."
sudo dnf install -y gstreamer1-libav gstreamer1-plugins-bad-freeworld gstreamer1-plugins-ugly

echo "Codec installation complete!"
echo ""
echo "Test your codecs:"
echo "  gst-inspect-1.0 | grep -E 'h264|mp4|aac'"
echo ""
echo "Then restart the browser and try playing a video."
