#!/usr/bin/env bash


cat OpenGD77_User_Guide.md | sed -e 's/<div style=\"page-break-after: always; break-after: page;\"><\/div>/\\pagebreak/g' -e 's/<!--.*{.*\(width=.*\).*}.*-->/{ \1 }/g' -e 's/<!--.*linebreak.*-->/\\newline/g' -e 's/<sup>\(.*\)<\/sup>/\\textsuperscript{\1}/g' -e 's/<sub>\(.*\)<\/sub>/\\textsubscript{\1}/g' > OpenGD77_User_Guide-workingcopy.md
pandoc \
    -H latex_preamble.tex \
    -V linkcolor:blue \
    -V geometry:a4paper \
    -V geometry:margin=1.5cm \
    -V mainfont="DejaVu Serif" \
    -V monofont="DejaVu Sans Mono" \
    -V fontsize="12pt" \
    --columns=16 \
    -V pagestyle=empty \
    --self-contained \
    -t latex -i OpenGD77_User_Guide-workingcopy.md -o OpenGD77_User_Guide.pdf
rm -f OpenGD77_User_Guide-workingcopy.md

exit 0
