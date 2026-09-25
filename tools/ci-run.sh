#!/usr/bin/env bash
#
# Start the firmware build + test workflow on GitHub and report the result.
# Nothing is compiled on this machine: the script starts a run, watches it, and
# tells you what came out of it.
#
# Two ways to start a run, picked automatically unless you force one:
#
#   dispatch  workflow_dispatch, the usual way (needs the actions:write scope)
#   push      push a throwaway ci-run-<timestamp> tag at the branch head; the
#             workflow listens for ci-run-* tags. Needs only the right to push,
#             so it works with a token that cannot create dispatch events. The
#             tag is deleted again when the script finishes.
#
#   tools/ci-run.sh                    # start a run for the current branch, watch
#   tools/ci-run.sh --no-watch         # start it and return immediately
#   tools/ci-run.sh --via push         # force the tag trigger
#   tools/ci-run.sh --branch main      # build another branch
#   tools/ci-run.sh --download out     # download the firmware when it succeeds
#   tools/ci-run.sh --list             # recent runs
#   tools/ci-run.sh --logs             # logs of the most recent run
#   tools/ci-run.sh --cancel           # cancel the most recent in-progress run
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/.." && pwd)"
workflow_file="build-test.yml"
tag_prefix="ci-run-"

step() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
note() { printf '    %s\n' "$*"; }
fail() { printf '\n\033[31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

usage() {
  cat <<'EOF'
Start the firmware build + test workflow on GitHub and report the result.

  tools/ci-run.sh                    # start a run for the current branch and watch it
  tools/ci-run.sh --no-watch         # start it, print the run URL, exit
  tools/ci-run.sh --branch main      # build another branch
  tools/ci-run.sh --via dispatch     # force workflow_dispatch (needs actions:write)
  tools/ci-run.sh --via push         # force the ci-run-* tag trigger
  tools/ci-run.sh --download DIR     # download the artifacts when the run succeeds
  tools/ci-run.sh --watch            # attach to the newest run without starting one
  tools/ci-run.sh --run 123456       # report on one specific run (no new run)
  tools/ci-run.sh --list             # recent runs for this workflow
  tools/ci-run.sh --logs             # logs of the most recent run
  tools/ci-run.sh --cancel           # cancel the most recent in-progress run
  tools/ci-run.sh --local [...]      # escape hatch: run build-and-test.sh here
                                     # instead; its arguments follow --local

Options:
  --repo OWNER/NAME     Repository (default: from the git remote)
  --branch NAME         Branch to build (default: the current git branch)
  --workflow FILE       Workflow to run (default: build-test.yml)
  --timeout SECONDS     Stop waiting after this long (default 1800)
  --run ID              Report on an existing run (implies --watch)
  --keep-tag            With --via push, leave the trigger tag in place
  -h, --help            This help

Exit code is 0 for a successful run, 1 for a failed or cancelled one.
EOF
}

branch=""
repo=""
timeout_s=1800
via="auto"
mode="run"
download_dir=""
keep_tag=0
run_id_arg=""

while [ $# -gt 0 ]; do
  case "$1" in
    --repo)     [ $# -ge 2 ] || fail "--repo needs a value"; repo="$2"; shift 2 ;;
    --branch)   [ $# -ge 2 ] || fail "--branch needs a value"; branch="$2"; shift 2 ;;
    --workflow) [ $# -ge 2 ] || fail "--workflow needs a value"; workflow_file="$2"; shift 2 ;;
    --timeout)  [ $# -ge 2 ] || fail "--timeout needs a value"; timeout_s="$2"; shift 2 ;;
    --via)      [ $# -ge 2 ] || fail "--via needs a value"; via="$2"; shift 2 ;;
    --download) [ $# -ge 2 ] || fail "--download needs a value"; download_dir="$2"; shift 2 ;;
    --run)      [ $# -ge 2 ] || fail "--run needs a run id"; run_id_arg="$2"; shift 2 ;;
    --keep-tag) keep_tag=1; shift ;;
    --no-watch) mode="start"; shift ;;
    --watch)    mode="watch"; shift ;;
    --list)     mode="list"; shift ;;
    --logs)     mode="logs"; shift ;;
    --cancel)   mode="cancel"; shift ;;
    # Everything after --local belongs to build-and-test.sh, so hand over now
    # rather than letting this parser reject its flags.
    --local)    shift; exec "$here/build-and-test.sh" "$@" ;;
    -h|--help)  usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

[ -z "$run_id_arg" ] || [ "$mode" != run ] || mode="watch"

case "$via" in auto|dispatch|push) ;; *) fail "--via expects auto, dispatch or push" ;; esac
case "$workflow_file" in
  build-test.yml)     workflow_name="Build and test" ;;
  espforge-build.yml) workflow_name="ESPForge firmware build" ;;
  *)                  workflow_name="$workflow_file" ;;
esac

command -v gh >/dev/null 2>&1 || fail "gh (GitHub CLI) not found. Install it, or use --local."
gh auth status >/dev/null 2>&1 || fail "gh is not authenticated. Run: gh auth login"

if [ -z "$repo" ]; then
  repo="$(git -C "$root" remote get-url origin 2>/dev/null \
          | sed -E 's#(https://github.com/|git@github.com:)##; s#\.git$##' || true)"
fi
[ -n "$repo" ] || fail "cannot work out the repository; pass --repo OWNER/NAME"

gh_args=(--repo "$repo")
gh() { command gh "${gh_args[@]}" "$@"; }

latest_run_id() {
  gh run list --workflow "$workflow_file" --limit 1 \
      --json databaseId --jq 'if length > 0 then .[0].databaseId else "" end' 2>/dev/null || true
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
      note "still $state after ${elapsed}s; leaving the run alive on GitHub"
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
    note "artifacts: gh run download $run_id --dir <dir>"
    return 0
  fi
  note "failed steps:"
  gh run view "$run_id" --json jobs \
    --jq '.jobs[].steps[] | select(.conclusion=="failure") | "    " + .name' 2>/dev/null || true
  note "compile errors are annotated on the run and in its job summary"
  note "raw log:   gh run view $run_id --log-failed"
  return 1
}

case "$mode" in
  list)
    step "Recent runs: $workflow_name ($repo)"
    gh run list --workflow "$workflow_file" --limit 10
    exit 0 ;;

  logs)
    id="${run_id_arg:-$(latest_run_id)}"
    [ -n "$id" ] || fail "no runs found for $workflow_file"
    step "Logs for run $id"
    gh run view "$id" --log
    exit 0 ;;

  cancel)
    id="${run_id_arg:-$(latest_run_id)}"
    [ -n "$id" ] || fail "no runs found for $workflow_file"
    step "Cancelling run $id"
    gh run cancel "$id"
    exit 0 ;;

  watch)
    id="${run_id_arg:-$(latest_run_id)}"
    [ -n "$id" ] || fail "no runs found for $workflow_file"
    step "Watching run $id"
    watch_run "$id" || true
    report_run "$id" || exit 1
    if [ -n "$download_dir" ]; then
      step "Downloading artifacts to $download_dir"
      gh run download "$id" --dir "$download_dir" || fail "could not download the artifacts"
      note "done"
    fi
    exit 0 ;;
esac

# ── work out what to build ───────────────────────────────────────────────────
if [ -z "$branch" ]; then
  branch="$(git -C "$root" rev-parse --abbrev-ref HEAD 2>/dev/null || true)"
fi
[ -n "$branch" ] || fail "cannot work out the branch; pass --branch NAME"

# The commit CI checks out must be one GitHub already has.
sha="$(git -C "$root" ls-remote --heads origin "$branch" 2>/dev/null | cut -f1 | head -1)"
[ -n "$sha" ] || fail "$branch is not on origin - push it first, CI cannot check it out"

step "$workflow_name on $repo"
note "branch    $branch"
note "commit    $sha"

# ── start the run ────────────────────────────────────────────────────────────
trigger_ref="$branch"     # what the run's head_branch will be
started_via=""

start_via_push() {
  tag="${tag_prefix}$(date -u +%Y%m%d-%H%M%S)"
  # Push straight from the remote commit: nothing is created in the local repo.
  git -C "$root" push --quiet origin "$sha:refs/tags/$tag" 2>/dev/null \
    || fail "could not push the trigger tag $tag. Pass --via dispatch instead."
  if [ "$keep_tag" -eq 0 ]; then
    note "trigger   tag $tag (deleted again when this script exits)"
    cleanup_tag() { git -C "$root" push --quiet --delete origin "refs/tags/$tag" >/dev/null 2>&1 || true; }
    trap cleanup_tag EXIT INT TERM
  else
    note "trigger   tag $tag (kept: --keep-tag)"
  fi
  trigger_ref="$tag"
  started_via="push"
}

start_via_dispatch() {
  # espforge-build.yml is the only workflow here that declares dispatch inputs.
  fields=()
  if [ "$workflow_file" = espforge-build.yml ]; then
    local uuid
    if command -v uuidgen >/dev/null 2>&1; then
      uuid="$(uuidgen | tr 'A-Z' 'a-z')"
    elif [ -r /proc/sys/kernel/random/uuid ]; then
      uuid="$(cat /proc/sys/kernel/random/uuid)"
    else
      uuid="$(date +%s)-$RANDOM"
    fi
    fields=(-f "espforge_build_uuid=$uuid" -f "espforge_source_commit=$sha")
    note "uuid      $uuid"
  fi
  local err
  err="$(mktemp)"
  if ! gh workflow run "$workflow_file" --ref "$branch" \
        ${fields[@]+"${fields[@]}"} 2> "$err"; then
    msg="$(cat "$err")"; rm -f "$err"
    case "$msg" in
      *"Resource not accessible by integration"*|*"403"*|*"404"*)
        printf '%s\n' "$msg" > /tmp/ci-run-dispatch-error
        return 1 ;;
      *) rm -f /tmp/ci-run-dispatch-error
         fail "dispatch failed: $msg
Does $workflow_file exist on the default branch?" ;;
    esac
  fi
  rm -f "$err" /tmp/ci-run-dispatch-error
  note "trigger   workflow_dispatch"
  started_via="dispatch"
}

case "$via" in
  dispatch) start_via_dispatch || fail "GitHub refused the dispatch: $(cat /tmp/ci-run-dispatch-error 2>/dev/null)

The token in use cannot create workflow_dispatch events (it needs actions:
write). Use --via push, run this from a machine where gh is authenticated as
you, or dispatch from the web:
  https://github.com/$repo/actions/workflows/$workflow_file" ;;
  push)     start_via_push ;;
  auto)
    if start_via_dispatch; then
      :
    else
      note "workflow_dispatch refused ($(head -c 80 /tmp/ci-run-dispatch-error 2>/dev/null) ...)"
      note "falling back to the tag trigger"
      start_via_push
    fi ;;
esac

if [ "$mode" = start ]; then
  note "started; find it with: tools/ci-run.sh --list"
  exit 0
fi

# ── find the run it created ──────────────────────────────────────────────────
# GitHub never returns the run id, so look for the newest run on the ref we just
# triggered. A little slack on the timestamp: GitHub's clock is not ours.
if since="$(date -u -d '-45 seconds' +%Y-%m-%dT%H:%M:%SZ 2>/dev/null)" && [ -n "$since" ]; then :
elif since="$(date -u -v-45S +%Y-%m-%dT%H:%M:%SZ 2>/dev/null)" && [ -n "$since" ]; then :
else since="$(date -u +%Y-%m-%dT%H:%M:%SZ)"; fi

step "Waiting for the run to appear"
run_id=""
for _ in $(seq 1 30); do
  run_id="$(gh run list --workflow "$workflow_file" --limit 20 \
              --json databaseId,headBranch,createdAt \
              --jq "[.[] | select(.headBranch == \"$trigger_ref\" and .createdAt >= \"$since\")] \
                    | sort_by(.createdAt) | if length > 0 then .[-1].databaseId else \"\" end" \
            2>/dev/null | head -1 || true)"
  [ -n "$run_id" ] && break
  sleep 4
done
[ -n "$run_id" ] || fail "no run appeared for $trigger_ref within ~2 minutes.
Check: gh run list --workflow $workflow_file"

url="$(gh run view "$run_id" --json url --jq .url 2>/dev/null || echo "(unknown)")"
step "Run $run_id"
note "$url"

step "Watching (up to ${timeout_s}s)"
watch_status=0
watch_run "$run_id" || watch_status=$?
if [ "$watch_status" -eq 124 ]; then
  note "not finished yet - the run keeps going on GitHub"
  exit 1
fi
report_run "$run_id" || exit 1
if [ -n "$download_dir" ]; then
  step "Downloading artifacts to $download_dir"
  gh run download "$run_id" --dir "$download_dir" || fail "could not download the artifacts"
  note "done"
fi
exit 0
