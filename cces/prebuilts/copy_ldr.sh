#!/bin/bash

# Function to copy .ldr files from either "Release" or "build" folders in SRC to DST
# For each project subfolder, checks build folder first (Makefile), then Release (CCES GUI)
# Warns if both exist in the same project as mixing build systems causes conflicts
copy_ldr_files() {
    local SRC="$1"
    local DST="$2"

    echo " Searching in: $SRC"
    echo " Destination:  $DST"
    mkdir -p "$DST"

    local files_copied=0
    local mixed_projects=()

    # Find all project directories (those containing either build or Release folders)
    local project_dirs=$(find "$SRC" -mindepth 1 -maxdepth 3 -type d \( -name "build" -o -name "Release" \) -exec dirname {} \; | sort -u)

    if [[ -z "$project_dirs" ]]; then
        echo " No build or Release folders found in $SRC"
        echo "----------------------------------"
        return
    fi

    # Process each project directory independently
    while IFS= read -r project_dir; do
        local project_name=$(basename "$project_dir")
        local build_ldrs=$(find "$project_dir/build" -maxdepth 2 -type f -name "*.ldr" 2>/dev/null)
        local release_ldrs=$(find "$project_dir/Release" -maxdepth 2 -type f -name "*.ldr" 2>/dev/null)

        # Warn if this specific project has both
        if [[ -n "$build_ldrs" && -n "$release_ldrs" ]]; then
            mixed_projects+=("$project_name")
        fi

        # Prioritize build over Release for this project
        local ldr_files=""
        local source_type=""
        
        if [[ -n "$build_ldrs" ]]; then
            ldr_files="$build_ldrs"
            source_type="build"
        elif [[ -n "$release_ldrs" ]]; then
            ldr_files="$release_ldrs"
            source_type="Release"
        fi

        # Copy files from this project
        if [[ -n "$ldr_files" ]]; then
            while IFS= read -r file; do
                filename=$(basename "$file")
                echo "  [$project_name/$source_type] Copying $filename → $DST"
                cp -f "$file" "$DST/$filename"
                ((files_copied++))
            done <<< "$ldr_files"
        fi
    done <<< "$project_dirs"

    # Show warnings for mixed build projects
    if [[ ${#mixed_projects[@]} -gt 0 ]]; then
        echo ""
        echo "WARNING: Mixed build systems detected in:"
        for proj in "${mixed_projects[@]}"; do
            echo "  - $proj (has both build/ and Release/ folders)"
        done
        echo "  Recommend: clean one build system before switching"
        echo "  - Makefile: make clean"
        echo "  - CCES GUI: Project → Clean"
        echo ""
    fi

    echo "Done: Copied $files_copied .ldr file(s) from $SRC"
    echo "----------------------------------"
}

copy_ldr_files "../examples" "."
copy_ldr_files "../Utils/flashing-tools/bootloader_sharcfx" "."

echo "All done!"
