#!/bin/bash

export PDFALTO_PATH=../
export PDFALTO_XSLT=../schema/alto2txt.xsl

# Function to process PDF files
process_pdf_files() {
  local input_dir="$1"
  local output_dir="$2"

  # Create the output directory if it doesn't exist
  mkdir -p "$output_dir"

  # Loop through all files and directories in the input directory
  for entry in "$input_dir"/*; do
    if [ -d "$entry" ]; then
      # If the entry is a directory, recursively process it
      local subdir_name=$(basename "$entry")
      process_pdf_files "$entry" "$output_dir/$subdir_name"
    elif [ -f "$entry" ] && [[ "$entry" == *.pdf ]]; then
      # If the entry is a PDF file, process it with pdfalto
      echo "Processing $entry"
      local filename=$(basename "$entry")
      ${PDFALTO_PATH}/./pdfalto -noImageInline -fullFontName -noImage -readingOrder "$entry" "$output_dir/${filename%.pdf}.xml"
      xsltproc ${PDFALTO_XSLT} "$output_dir/${filename%.pdf}.xml" > "$output_dir/${filename%.pdf}.txt"
    fi
  done
}

# Main script execution
input_directory="$1"
output_directory="$2"

if [ -z "$input_directory" ] || [ -z "$output_directory" ]; then
  echo "Usage: $0 <input_directory> <output_directory>"
  exit 1
fi

process_pdf_files "$input_directory" "$output_directory"
