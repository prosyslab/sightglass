#!/usr/bin/env fish

argparse 'n/name=' 'v/variant=' -- $argv
or return

if test (count $_flag_variant) -eq 0
  echo "variant is not specified (-v or --variant)" >&2
  exit 1
end

set -l NAME "bench"
set -ql $_flag_name[1]; and set -l NAME $_flag_name[-1]

set -l variant $_flag_variant[-1]
set -l fire_file $variant.opts.fire

############# CONFIG ##############

echo -n "Total Rule Fires: " >&2
rg "Fire" $variant.$NAME.stderr -c >&2
rg "Fire.*opts" $variant.$NAME.stderr | cut -d' ' -f4,6 | sort -n > $fire_file

echo -n "Total Opt Rule Fires: " >&2
wc -l $fire_file >&2

set -l CODEGEN_ROOT /home/user/transopt/third_party/wasmtime/cranelift/codegen
pushd $CODEGEN_ROOT
git switch bench-$variant
popd

echo "File,Line,Count"
uniq -c $fire_file | sort -nr | while read -l count file lineno
  set -l basename (path basename $file)
  set -l filepath "$CODEGEN_ROOT/$file"
  set -l lineno (math $lineno + 1)
  set -l snippet (sed -n "$lineno,+50p" $filepath | sexps.py --first)

  printf "%s,%s,%s,%s\n" $basename $lineno $count "$snippet"
end


# awk -v OFS=, '{sub(".*/", "", $2); print $2, $3, $1}'

