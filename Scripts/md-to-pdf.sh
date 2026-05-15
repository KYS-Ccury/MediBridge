#!/usr/bin/env bash
# md-to-pdf.sh — MD → HTML → PDF (wkhtmltopdf)
# 사용: bash Scripts/md-to-pdf.sh <input.md> <output.pdf>
set -e
IN="$1"
OUT="$2"
TMP_HTML=$(mktemp --suffix=.html)

# MD → HTML (단순 변환 — python markdown 모듈 사용)
python3 -c "
import sys, re
import markdown
with open(sys.argv[1], 'r', encoding='utf-8') as f:
    md = f.read()
html_body = markdown.markdown(md, extensions=['tables', 'fenced_code', 'sane_lists'])
print(f'''<!DOCTYPE html>
<html lang=\"ko\"><head><meta charset=\"utf-8\"/><title>MediBridge Production Test</title>
<style>
@page {{ size: A4; margin: 18mm 14mm; }}
body {{
  font-family: 'Malgun Gothic', 'Noto Sans CJK KR', sans-serif;
  color: #1A2238; line-height: 1.55; font-size: 11pt;
}}
h1 {{ color: #0F4C81; font-size: 18pt; border-bottom: 3px solid #0F4C81; padding-bottom: 6px; }}
h2 {{ color: #0F4C81; font-size: 14pt; margin-top: 24px; border-bottom: 1px solid #D5DCE4; padding-bottom: 4px; }}
h3 {{ color: #1A2238; font-size: 12pt; margin-top: 16px; }}
h4 {{ color: #5B6478; font-size: 11pt; }}
code {{ background: #F1F4F8; padding: 1px 4px; border-radius: 3px; font-family: Consolas, monospace; font-size: 10pt; }}
pre {{ background: #F1F4F8; padding: 8px 10px; border-radius: 6px; overflow-x: auto; font-size: 9pt; }}
table {{ border-collapse: collapse; width: 100%; margin: 8px 0; }}
th, td {{ border: 1px solid #D5DCE4; padding: 5px 8px; vertical-align: top; font-size: 10pt; }}
th {{ background: #EAF1F8; font-weight: bold; }}
tr:nth-child(even) td {{ background: #FAFBFC; }}
blockquote {{ border-left: 4px solid #0F4C81; padding: 4px 10px; background: #F0F7FF; margin: 8px 0; }}
ul, ol {{ padding-left: 22px; }}
hr {{ border: 0; border-top: 1px solid #D5DCE4; margin: 16px 0; }}
.passed {{ color: #2E7D32; font-weight: bold; }}
.failed {{ color: #C62828; font-weight: bold; }}
</style></head><body>{html_body}</body></html>''')
" "$IN" > "$TMP_HTML"

# HTML → PDF (wkhtmltopdf)
wkhtmltopdf \
    --quiet \
    --encoding utf-8 \
    --enable-local-file-access \
    --margin-top 18mm --margin-bottom 18mm --margin-left 14mm --margin-right 14mm \
    --footer-center "[page] / [topage]" --footer-font-size 9 \
    "$TMP_HTML" "$OUT"

rm -f "$TMP_HTML"
echo "PDF 생성 완료: $OUT"
ls -la "$OUT"
