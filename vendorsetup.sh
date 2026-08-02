#!/bin/bash
# Keep the camera BLOB-buffer gralloc fix with this device tree.
"$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/apply-gralloc-blob-patch.sh"
