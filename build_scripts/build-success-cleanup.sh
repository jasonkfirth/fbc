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

fb_remove_build_tree() {
    fb_root="$1"
    fb_path="$2"

    [ -n "$fb_root" ] || return 1
    [ -n "$fb_path" ] || return 1
    [ "$fb_path" != "/" ] || return 1
    [ "$fb_path" != "$fb_root" ] || return 1
    [ "$fb_path" != "$fb_root/" ] || return 1

    [ -e "$fb_path" ] || return 0

    if rm -rf "$fb_path" 2>/dev/null; then
        return 0
    fi

    case "$fb_path" in
        "$fb_root"/*)
            fb_rel="${fb_path#"$fb_root"/}"
            ;;
        *)
            return 1
            ;;
    esac

    case "$fb_rel" in
        ""|"."|".."|../*|/*)
            return 1
            ;;
    esac

    if command -v docker >/dev/null 2>&1 && docker ps >/dev/null 2>&1; then
        fb_docker_platform=""

        case "$(uname -m 2>/dev/null || echo unknown)" in
            x86_64|amd64)
                fb_docker_platform="linux/amd64"
                ;;
            aarch64|arm64)
                fb_docker_platform="linux/arm64"
                ;;
            armv7*|armv7l)
                fb_docker_platform="linux/arm/v7"
                ;;
            i386|i486|i586|i686)
                fb_docker_platform="linux/386"
                ;;
            ppc64le)
                fb_docker_platform="linux/ppc64le"
                ;;
            s390x)
                fb_docker_platform="linux/s390x"
                ;;
            riscv64)
                fb_docker_platform="linux/riscv64"
                ;;
        esac

        if [ -n "$fb_docker_platform" ]; then
            set -- --platform "$fb_docker_platform"
        else
            set --
        fi

        docker run --rm "$@" \
            -v "$fb_root:/work" \
            -w /work \
            alpine:3.23 \
            sh -c 'for fb_path do rm -rf "$fb_path"; done' \
            sh "$fb_rel"
        return $?
    fi

    if command -v sudo >/dev/null 2>&1; then
        sudo -n rm -rf "$fb_path"
        return $?
    fi

    return 1
}
