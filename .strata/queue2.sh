#!/bin/bash
# Like queue.sh but re-scans .strata/tasks after every task, so tasks added while it runs are picked up.
cd /home/rhy/drmariowii/repo
while true; do
  t=$(ls .strata/tasks/ | sort | while read x; do grep -q " ${x%.md} OK" .strata/status.txt 2>/dev/null || { echo $x; break; }; done)
  [ -z "$t" ] && { echo "$(date +%T) queue empty" >> .strata/status.txt; exit 0; }
  .strata/run-step.sh "$t" || exit 1
done
