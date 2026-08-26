#!/usr/bin/env bash
# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

set -euo pipefail

# Script: prepare_release_notes.sh
# Purpose: Extract release note sections from RST file and convert to Markdown format
# Usage: ./scripts/prepare_release_notes.sh <release_body.md> <release_note.rst>
# Environment: Expects release_body.md to contain $SECTION_NAME placeholders

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <release_body_file> <release_note_rst_file>" >&2
  exit 1
fi

RELEASE_BODY="$1"
RELEASE_NOTE="$2"

if [[ ! -f "$RELEASE_BODY" ]]; then
  echo "Error: Release body file not found: $RELEASE_BODY" >&2
  exit 1
fi

if [[ ! -f "$RELEASE_NOTE" ]]; then
  echo "Warning: Release note file not found: $RELEASE_NOTE" >&2
  echo "Skipping release note extraction (sections will remain as placeholders)"
  exit 0
fi

# ---------------------
# Helper Functions
# ---------------------

# Extract a section between two RST headings (underline style)
# Uses sed to find the section and remove heading/underline lines
extract_section() {
  local heading="$1"
  local next_heading="${2:-}"
  local file="$3"
  
  if [[ -z "$next_heading" ]]; then
    # Extract from heading to next heading at same level
    sed -n "/^${heading}$/,/^[=-]\{1,\}$/p" "$file" | sed '1,2d;$d'
  else
    # Extract from heading to specific next heading
    sed -n "/^${heading}$/,/^${next_heading}$/p" "$file" | sed '1,2d;$d'
  fi
}

# Convert RST inline links to Markdown format
# RST:      `link text <https://example.com>`_
# Markdown: [link text](https://example.com)
convert_rst_links() {
  sed "s/\`\([^<]*\) <\([^>]*\)>\`_/[\1](\2)/g"
}

# ---------------------
# Extract Sections
# ---------------------

declare -A SECTIONS

# Define all release note sections in order
# Key: section name (used in template)
# Value: "heading_in_rst" "next_heading_in_rst" (or "" for last section)
declare -A SECTION_MAP=(
  [NEW_FEATURES]="New Features|Improvements"
  [IMPROVEMENTS]="Improvements|Bug Fixes"
  [BUG_FIXES]="Bug Fixes|Other Changes by Label"
  [OTHER_CHANGES]="Other Changes by Label|Compatibility"
  [COMPATIBILITY]="Compatibility|Performed Verification"
  [PERFORMED_VERIFICATION]="Performed Verification|Known Issues"
  [KNOWN_ISSUES]="Known Issues|Known Vulnerabilities"
  [KNOWN_VULNERABILITIES]="Known Vulnerabilities|Upgrade Instructions"
  [UPGRADE_INSTRUCTIONS]="Upgrade Instructions|"
)

# Extract each section and convert links
for section_var in "${!SECTION_MAP[@]}"; do
  IFS='|' read -r heading next_heading <<< "${SECTION_MAP[$section_var]}"
  
  if [[ -z "$next_heading" ]]; then
    SECTIONS[$section_var]=$(extract_section "$heading" "" "$RELEASE_NOTE" | convert_rst_links)
  else
    SECTIONS[$section_var]=$(extract_section "$heading" "$next_heading" "$RELEASE_NOTE" | convert_rst_links)
  fi
done

# ---------------------
# Update Release Body
# ---------------------

# Replace all section placeholders using envsubst
# Must export all variables for envsubst to find them
for section_var in "${!SECTIONS[@]}"; do
  export "$section_var"="${SECTIONS[$section_var]}"
done

# Create temporary file with all sections substituted
envsubst "$(printf '$%s ' "${!SECTIONS[@]}")" < "$RELEASE_BODY" > "${RELEASE_BODY}.tmp"
mv "${RELEASE_BODY}.tmp" "$RELEASE_BODY"

echo "✅ Release notes prepared successfully"
