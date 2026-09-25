#!/usr/bin/env bash
#
# Run the ESPForge firmware build workflow on GitHub for a branch, then report
# the result. The workflow only triggers on workflow_dispatch/repository_dispatch,
# so a push alone never builds anything - this is how you make it build.
#
#   tools/ci-run.sh                    # dispatch for the current branch and watch
#   tools/ci-run.sh --no-watch         # dispatch and return immediately
#   tools/ci-run.sh --branch main      # dispatch for another branch
#   tools/ci-run.sh --list             # recent runs
#   tools/ci-run.sh --logs             # logs of the most recent run
#   tools/ci-run.sh --cancel           # cancel the most recent in-progress run
#   tools/ci-run.sh --local            # same steps, but run here (no GitHub)
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workflow_file="espforge-build.yml"
workflow_name="ESPForge firmware build"

step() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
note() { printf '    %s\n' "$*"; }
fail() { printf '\n\033[31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

usage() {
  cat <<'EOF'
Run the ESPForge firmware build workflow on GitHub and report the result.

  tools/ci-run.sh                    # dispatch for the current branch and watch it
  tools/ci-run.sh --no-watch         # dispatch, print the run URL, exit
  tools/ci-run.sh --branch main      # dispatch for another branch
  tools/ci-run.sh --list             # recent runs for this workflow
  tools/ci-run.sh --logs             # logs of the most recent run
  tools/ci-run.sh --cancel           # cancel the most recent in-progress run
  tools/ci-run.sh --local            # run the same steps here instead of on GitHub
  tools/ci-run.sh --watch            # attach to the most recent run without dispatching

Options:
  --repo OWNER/NAME     Repository (default: from the git remote)
  --branch NAME         Branch to build (default: the current git branch)
  --uuid UUID           Build identifier (default: generated)
  --timeout SECONDS     Stop waiting after this long (default 1800)
  -h, --help            This help

Exit code is 0 for a successful run, 1 for a failed or cancelled one.
EOF
}

branch=""
repo=""
uuid=""
timeout_s=1800
mode="dispatch"

while [ $# -gt 0 ]; do
  case "$1" in
    --repo)     [ $# -ge 2 ] || fail "--repo needs a value"; repo="$2"; shift 2 ;;
    --branch)   [ $# -ge 2 ] || fail "--branch needs a value"; branch="$2"; shift 2 ;;
    --uuid)     [ $# -ge 2 ] || fail "--uuid needs a value"; uuid="$2"; shift 2 ;;
    --timeout)  [ $# -ge 2 ] || fail "--timeout needs a value"; timeout_s="$2"; shift 2 ;;
    --no-watch) mode="dispatch-nowait"; shift ;;
    --watch)    mode="watch"; shift ;;
    --list)     mode="list"; shift ;;
    --logs)     mode="logs"; shift ;;
    --cancel)   mode="cancel"; shift ;;
    --local)    mode="local"; shift ;;
    -h|--help)  usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

# ── local mode needs nothing from GitHub ─────────────────────────────────────
if [ "$mode" = local ]; then
  exec "$here/build-and-test.sh"
fi

command -v gh >/dev/null 2>&1 || fail "gh (GitHub CLI) not found. Install it, or use --local."
gh auth status >/dev/null 2>&1 || fail "gh is not authenticated. Run: gh auth login"

if [ -z "$repo" ]; then
  repo="$(git -C "$here/.." remote get-url origin 2>/dev/null \
          | sed -E 's#(https://github.com/|git@github.com:)##; s#\.git$##' || true)"
fi
[ -n "$repo" ] || fail "cannot work out the repository; pass --repo OWNER/NAME"

gh_args=(--repo "$repo")
gh() { command gh "${gh_args[@]}" "$@"; }

latest_run_id() {  # extra args are passed through as gh run list filters
  gh run list --workflow "$workflow_file" --limit 1 \
      --json databaseId --jq 'if length > 0 then .[0].databaseId else "" end' "$@" 2>/dev/null || true
}

# Poll a run until it finishes. gh 2.23's `run watch` has no timeout flag, so
# this loop owns the deadline and still prints progress.
watch_run() {
  run_id="$1"
  started="$(date +%s)"
  while :; do
    line="$(gh run view "$run_id" --json status,conclusion \
              --jq '.status + "|" + (.conclusion // "")' 2>/dev/null || echo "queued|")"
    state="${line%%|*}"
    conclusion="${line#*|}"
    elapsed=$(( $(date +%s) - started ))
    if [ "$state" = completed ]; then
      printf '    [%4ds] completed: %s\n' "$elapsed" "$conclusion"
      return 0
    fi
    if [ "$elapsed" -ge "$timeout_s" ]; then
      note "still $state after ${elapsed}s; keeping the run alive on GitHub"
      note "re-attach with: tools/ci-run.sh --watch"
      return 124
    fi
    current="$(gh run view "$run_id" --json jobs \
                 --jq '[.jobs[].steps[] | select(.status=="in_progress") | .name] | join(", ")' \
                 2>/dev/null || true)"
    printf '    [%4ds] %s%s\n' "$elapsed" "$state" "${current:+ - $current}"
    sleep 15
  done
}

report_run() {
  run_id="$1"
  conclusion="$(gh run view "$run_id" --json conclusion --jq '.conclusion // "unknown"' 2>/dev/null || echo unknown)"
  step "Result: $conclusion"
  if [ "$conclusion" = success ]; then
    note "artifacts: gh run download $run_id"
    exit 0
  fi
  note "failed steps:"
  gh run view "$run_id" --json jobs \
    --jq '.jobs[].steps[] | select(.conclusion=="failure") | "    " + .name' 2>/dev/null || true
  note "failure log: gh run view $run_id --log-failed"
  exit 1
}

case "$mode" in
  list)
    step "Recent runs: $workflow_name ($repo)"
    gh run list --workflow "$workflow_file" --limit 10
    exit 0 ;;

  logs)
    id="$(latest_run_id)"
    [ -n "$id" ] || fail "no runs found for $workflow_file"
    step "Logs for run $id"
    gh run view "$id" --log
    exit 0 ;;

  cancel)
    id="$(latest_run_id)"
    [ -n "$id" ] || fail "no runs found for $workflow_file"
    step "Cancelling run $id"
    gh run cancel "$id"
    exit 0 ;;

  watch)
    id="$(latest_run_id)"
    [ -n "$id" ] || fail "no runs found for $workflow_file"
    step "Watching run $id"
    watch_run "$id"
    report_run "$id" ;;
esac

# ── dispatch ─────────────────────────────────────────────────────────────────
if [ -z "$branch" ]; then
  branch="$(git -C "$here/.." rev-parse --abbrev-ref HEAD 2>/dev/null || true)"
fi
[ -n "$branch" ] || fail "cannot work out the branch; pass --branch NAME"

sha="$(git -C "$here/.." ls-remote --heads origin "$branch" 2>/dev/null | cut -f1 | head -1)"
if [ -n "$sha" ]; then
  note "resolved $branch on origin"
else
  sha="$(git -C "$here/.." rev-parse --verify --quiet "$branch^{commit}" || true)"
  [ -n "$sha" ] || fail "cannot resolve branch $branch to a commit"
  note "warning: $branch is not on origin - push it first or the CI checkout will fail"
fi

if [ -z "$uuid" ]; then
  if command -v uuidgen >/dev/null 2>&1; then
    uuid="$(uuidgen | tr 'A-Z' 'a-z')"
  elif [ -r /proc/sys/kernel/random/uuid ]; then
    uuid="$(cat /proc/sys/kernel/random/uuid)"
  else
    uuid="$(date +%s)-$RANDOM"
  fi
fi

step "Dispatch: $workflow_name"
note "repo      $repo"
note "branch    $branch"
note "commit    $sha"
note "uuid      $uuid"

dispatch_err="$(mktemp)"
if ! gh workflow run "$workflow_file" --ref "$branch" \
      -f espforge_build_uuid="$uuid" \
      -f espforge_source_commit="$sha" 2> "$dispatch_err"; then
  msg="$(cat "$dispatch_err")"
  rm -f "$dispatch_err"
  case "$msg" in
    *"Resource not accessible by integration"*|*"403"*)
      fail "GitHub refused the dispatch: $msg

The token in use cannot create workflow_dispatch events (it needs actions:
write). Run this script from your own machine, where gh is authenticated as
you, or dispatch from the web:
  https://github.com/$repo/actions/workflows/$workflow_file" ;;
    *)
      fail "dispatch failed: $msg
Does $workflow_file exist on the default branch?" ;;
  esac
fi
rm -f "$dispatch_err"

if [ "$mode" = "dispatch-nowait" ]; then
  note "dispatched; find it with: tools/ci-run.sh --list"
  exit 0
fi

# GitHub creates the run asynchronously and does not return its id, so find it
# by the uuid that the workflow puts in its run name.
step "Waiting for the run to appear"
run_id=""
for _ in $(seq 1 30); do
  run_id="$(gh run list --workflow "$workflow_file" --branch "$branch" --limit 10 \
              --json databaseId,displayTitle \
              --jq ".[] | select(.displayTitle | contains(\"$uuid\")) | .databaseId" \
            | head -1)"
  [ -n "$run_id" ] && break
  sleep 4
done
[ -n "$run_id" ] || fail "the run did not appear within ~2 minutes. Check: gh run list --workflow $workflow_file"

url="$(gh run view "$run_id" --json url --jq .url 2>/dev/null || echo "(unknown)")"
step "Run $run_id"
note "$url"

step "Watching (up to ${timeout_s}s)"
watch_run "$run_id" || true
report_run "$run_id"
