#!/bin/sh

fb_cleanup_success() {
    fb_root="$1"
    fb_make_cmd="${2:-}"
    shift 2

    echo
    echo "==> cleaning build artefacts"

    if [ -n "$fb_root" ] && [ -d "$fb_root" ]; then
        (
            cd "$fb_root" || exit 0

            if [ -n "$fb_make_cmd" ] && command -v "$fb_make_cmd" >/dev/null 2>&1; then
                "$fb_make_cmd" clean >/dev/null 2>&1 || true
            fi

            rm -rf stage
            rm -rf stage/bootstrap-dist
            rm -rf .build-*
            rm -rf .maketests-tmp maketests-log test-run-log
            rm -rf src/*/obj
            rm -f src/compiler/*.c src/compiler/*.asm
        ) || true
    fi

    for fb_path in "$@"; do
        [ -n "$fb_path" ] || continue

        case "$fb_path" in
            /|"$fb_root"|"$fb_root/")
                continue
                ;;
        esac

        rm -rf "$fb_path" 2>/dev/null || true
    done
}
