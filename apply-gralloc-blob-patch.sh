#!/bin/bash
# Keep the GM9 Pro camera BLOB layer count fix immediately before METADATA_V2.
set -e

device_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
android_top="$(cd "${device_dir}/../../.." && pwd)"
source_file="${android_top}/hardware/qcom-caf/sdm660/display/gralloc/gr_buf_mgr.cpp"

if [ ! -f "${source_file}" ]; then
  echo "GM9PRO: display source is unavailable; skipping camera gralloc patch."
  exit 0
fi

blob_block=$'  if (format == HAL_PIXEL_FORMAT_BLOB) {\n    hnd->layer_count = data.size;\n  }'
correct_sequence="${blob_block}"$'\n\n#ifdef METADATA_V2\n  auto error = validateAndMap(hnd, descriptor.GetReservedSize());'
legacy_sequence=$'#ifdef METADATA_V2\n'"${blob_block}"$'\n\n  auto error = validateAndMap(hnd, descriptor.GetReservedSize());'
clean_sequence=$'#ifdef METADATA_V2\n  auto error = validateAndMap(hnd, descriptor.GetReservedSize());'

contains() {
  NEEDLE="$1" perl -0ne 'exit((index($_, $ENV{NEEDLE}) >= 0) ? 0 : 1)' "$2"
}

if contains "${correct_sequence}" "${source_file}"; then
  echo "GM9PRO: camera gralloc BLOB patch is already correctly applied."
elif contains "${legacy_sequence}" "${source_file}"; then
  OLD="${legacy_sequence}" NEW="${correct_sequence}" perl -0pi -e 's/\Q$ENV{OLD}\E/$ENV{NEW}/' "${source_file}"
  echo "GM9PRO: moved camera gralloc BLOB patch before METADATA_V2."
elif contains "${clean_sequence}" "${source_file}"; then
  OLD="${clean_sequence}" NEW="${blob_block}"$'\n\n'"${clean_sequence}" perl -0pi -e 's/\Q$ENV{OLD}\E/$ENV{NEW}/' "${source_file}"
  echo "GM9PRO: applied camera gralloc BLOB patch before METADATA_V2."
else
  echo "GM9PRO: camera gralloc source did not match an expected layout; leaving it unchanged." >&2
fi
