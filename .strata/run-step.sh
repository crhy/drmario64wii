#!/bin/bash
# Runs ONE task prompt (.strata/tasks/NN-*.md) through OpenCode on Strata, then its gate
# (the task's line starting "GATE:"). One retry if nothing changed, one fix round if the gate fails.
cd /home/rhy/drmariowii/repo || exit 1
export DEVKITPRO=$HOME/devkitpro DEVKITPPC=$HOME/devkitpro/devkitPPC
export PATH=/home/rhy/drmariowii/repo/.strata/bin:$PATH  # git guard (blocks checkout/reset/stash...)
D=.strata; STATUS=$D/status.txt; f=$D/tasks/$1; n=$(basename "$f" .md)
[ -f "$f" ] || { echo "no such task: $f" >&2; exit 2; }
mkdir -p $D/logs $D/scratch
GATECMD=$(grep -m1 '^GATE:' "$f" | cut -d: -f2-); GATE=$D/logs/$n.gate.txt
gate() { [ -z "$GATECMD" ] && return 0; timeout 900 bash -c "$GATECMD" >"$GATE" 2>&1; }
state() { { git diff; git ls-files -o --exclude-standard -z | xargs -0 cat 2>/dev/null; cat $D/NOTES.md 2>/dev/null; } | sha1sum; }
run() {
  LOG=$D/logs/$n-$(date +%H%M%S).log; ln -sf "$(basename $LOG)" $D/logs/latest.log
  echo "$(date +%T) START $1" >> "$STATUS"; printf '%s\n' "$2" > $D/_note.md
  if [ "${3:-}" = continue ]; then
    timeout 2700 /home/rhy/.opencode/bin/opencode run --continue \
      "Proceed now: write the files with the write tool, then run the GATE. Do not ask for permission and do not reply with text until the GATE passes. $2" > "$LOG" 2>&1 &
  else
  timeout 2700 /home/rhy/.opencode/bin/opencode run --title "$1" \
    "Read these three files with the read tool, then carry out the task they describe exactly: $PWD/$D/_header.md , $PWD/$f , $PWD/$D/_note.md (extra notes for this attempt; may be empty)." > "$LOG" 2>&1 &
  fi
  pid=$!; ( sleep 180; [ -s "$LOG" ] || { echo "$(date +%T) $1 hung, killed" >> "$STATUS"; kill $pid; } ) & g=$!
  wait $pid; echo "exit=$?" >> "$LOG"; kill $g 2>/dev/null
}
# Snapshot every changed/untracked file before the task, so a regression can be bisected (.strata/snapshots/<task>.tgz).
mkdir -p $D/snapshots; { git diff --name-only; git ls-files -o --exclude-standard | grep -v '^\.strata/'; } | sort -u | tar czf $D/snapshots/$n-$(date +%m%d%H%M).tgz -T - 2>/dev/null
before=$(state); run "drmario $n" ""
if [ "$before" = "$(state)" ]; then
  run "drmario $n (continue)" "" continue
  [ "$before" = "$(state)" ] && run "drmario $n (retry)" "A previous attempt wrote nothing. Write the first file in your FIRST reply."
  [ "$before" = "$(state)" ] && { echo "$(date +%T) $n no changes twice, STOP" >> "$STATUS"; exit 1; }
fi
for i in 1 2; do
  gate && { echo "$(date +%T) $n OK" >> "$STATUS"; exit 0; }
  echo "$(date +%T) $n gate FAILED ($i)" >> "$STATUS"
  run "drmario $n (fix $i)" "The task was attempted but its gate fails: $GATECMD
Fix only what is needed (keep the intended behaviour). Gate output tail:
$(tail -60 "$GATE")"
done
gate && { echo "$(date +%T) $n OK" >> "$STATUS"; exit 0; }
echo "$(date +%T) $n gate FAILED, STOP" >> "$STATUS"; exit 1
