#!/usr/bin/env python3
"""Summarise a run_sg_all.sh run: summary.txt plus an index.html thumbnail grid.

Usage: sg_report.py <outdir>   (outdir holds <id>.png and log/<id>.txt)
"""
import glob, html, os, re, sys

out = sys.argv[1]
rows = []
for log in sorted(glob.glob(os.path.join(out, 'log', '*.txt'))):
    rid = os.path.splitext(os.path.basename(log))[0]
    text = open(log, errors='replace').read()
    rom = re.search(r'^rom: (.*)$', text, re.M)
    rc = re.search(r'^exit: (\d+)$', text, re.M)
    peak = re.search(r'heap: peak=(\d+)', text)
    rows.append({
        'id': rid,
        'rom': rom.group(1) if rom else '?',
        'exit': int(rc.group(1)) if rc else -1,
        'asan': 'AddressSanitizer' in text,
        'panic': 'PANIC:' in text,
        'state_fail': 'state round-trip at frame' in text and ': ok' not in text,
        'adaptor': re.findall(r'SG: RAM adaptor detected at (\$[0-9A-F]{4})', text),
        'peak': int(peak.group(1)) if peak else 0,
    })
rows.sort(key=lambda r: r['rom'])

def flags(r):
    f = []
    if r['exit'] != 0: f.append('exit=%d' % r['exit'])
    if r['asan']: f.append('ASAN')
    if r['panic']: f.append('PANIC')
    if r['state_fail']: f.append('STATE')
    if r['adaptor']: f.append('RAM@' + ','.join(sorted(set(r['adaptor']))))
    return f

bad = [r for r in rows if r['exit'] != 0 or r['asan'] or r['panic'] or r['state_fail']]
with open(os.path.join(out, 'summary.txt'), 'w') as s:
    s.write('roms: %d  failures: %d  max heap peak: %d\n\n' %
            (len(rows), len(bad), max((r['peak'] for r in rows), default=0)))
    for r in rows:
        f = flags(r)
        if f:
            s.write('%-14s %s\n' % (' '.join(f), r['rom']))

with open(os.path.join(out, 'index.html'), 'w') as h:
    h.write('<!doctype html><meta charset="utf-8"><title>SG-1000 host run</title>'
            '<style>body{font:12px sans-serif;background:#222;color:#ddd}'
            '.g{display:flex;flex-wrap:wrap;gap:8px}.c{width:256px}'
            '.c img{width:256px;image-rendering:pixelated;display:block}'
            '.b{color:#f66;font-weight:bold}</style>\n')
    h.write('<p>%d ROMs, %d failures</p><div class="g">\n' % (len(rows), len(bad)))
    for r in rows:
        f = flags(r)
        name = html.escape(os.path.basename(r['rom']))
        h.write('<div class="c"><img src="%s.png" loading="lazy"><div>%s</div>'
                '<div class="b">%s</div></div>\n' % (r['id'], name, html.escape(' '.join(f))))
    h.write('</div>\n')

print(open(os.path.join(out, 'summary.txt')).read())
