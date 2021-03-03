#!/usr/bin/env bash
#see http://www.xpdfreader.com/download.html, read the README in archive file for more instructions
set -e
base_dir="/usr/local/share/xpdf"
# not working for pdfplato when xpdfrc in /usr/local/etc/xpdfrc
xpdfrc="${HOME}/.xpdfrc"
#for debug
#base_dir="."
#xpdfrc="./xpdfrc"

languages=(
  "arabic"
  "chinese-simplified"
  "chinese-traditional"
  "cyrillic"
  "greek"
  "hebrew"
  "japanese"
  "korean"
  "latin2"
  "thai"
  "turkish"
)
for language in "${languages[@]}" ; do
  language_dir="${base_dir}/${language}"
  sudo mkdir -p "${language_dir}"
  language_url="https://dl.xpdfreader.com/xpdf-${language}.tar.gz"
  wget -O- "${language_url}" | sudo tar --strip-components 1 -xvzf - -C "${language_dir}"
  cat "${language_dir}/add-to-xpdfrc" >> "${xpdfrc}"
done
