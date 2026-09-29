#!/usr/bin/env python3
"""
Checks the Czech translation table (src/lang_cs.cpp) against the texts
marked with _("...") / N_("...") in src/.

    python tests/check_translations.py [--missing]

Fails when
  - a marked text has no translation,
  - the table has a text that isn't used anywhere (stale entry),
  - a text is in the table twice,
  - a translation has different printf conversions than the English text
    (the same ones in the same order - otherwise printf would crash),
  - a translation isn't valid UTF-8 or uses letters the font can't show.

--missing prints the untranslated texts as table entries to fill in.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'src')
TABLE = os.path.join(SRC, 'lang_cs.cpp')

# Texts that are the same in every language on purpose (key names)
SAME = {'Tab', 'N', 'D', 'CTRL+X', 'F1', 'F2', 'F3', 'ESC', 'CTRL+N', 'f2', 'CTRL+F2',
        'F9', 'P', 'SHIFT+R', 'CTRL+U', 'CTRL+S', 'B', '1', '2', '3', '4', 'CTRL+1',
        'CTRL+2', 'CTRL+3', 'CTRL+4', 'F10', 'PgUp', 'PgDown', 'Home', 'End',
        'I:%d V:%d R:%d L:%d', '. . . . . . . . demo',
        'Berusky v.%s (C) Anakreon 2006, http://www.anakreon.cz/\n'}

# Letters the font can draw: ASCII + Czech (lang.cpp glyph_decompose)
CZECH = set('áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ')

LITERAL = r'"((?:[^"\\\n]|\\.)*)"'
MARK = re.compile(r'\bN?_\(\s*((?:' + LITERAL + r'\s*)+)\)')
ENTRY = re.compile(r'\{\s*((?:' + LITERAL + r'\s*)+),\s*((?:' + LITERAL + r'\s*)+)\}')
PRINTF = re.compile(r'%[-+ #0]*[0-9*]*(?:\.[0-9*]+)?(?:hh|h|ll|l|z)?[diouxXcsfgeEp%]')


def unescape(s):
    out = []
    i = 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            n = s[i + 1]
            out.append({'n': '\n', 't': '\t', 'r': '\r', '"': '"', "'": "'", '\\': '\\'}.get(n, '\\' + n))
            i += 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def literals(group):
    return ''.join(unescape(m.group(1)) for m in re.finditer(LITERAL, group))


def strip_comments(src):
    """Removes // and /* */ comments, keeps string literals and line numbers."""
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == '\\' else 1
            out.append(src[i:j + 1])
            i = j + 1
        elif src.startswith('//', i):
            j = src.find('\n', i)
            i = n if j < 0 else j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append('\n' * src.count('\n', i, j))
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def source_texts():
    texts = {}
    for name in sorted(os.listdir(SRC)):
        if not name.endswith(('.cpp', '.h')) or name in ('lang_cs.cpp',):
            continue
        with open(os.path.join(SRC, name), encoding='utf-8', errors='replace') as f:
            src = f.read()
        # line continuation inside string literals
        src = src.replace('\\\r\n', '').replace('\\\n', '')
        src = strip_comments(src)
        for m in MARK.finditer(src):
            line = src.count('\n', 0, m.start()) + 1
            texts.setdefault(literals(m.group(1)), '%s:%d' % (name, line))
    return texts


def table_entries():
    with open(TABLE, encoding='utf-8') as f:
        src = f.read()
    src = src.replace('\\\r\n', '').replace('\\\n', '')
    return [(literals(m.group(1)), literals(m.group(3)), src.count('\n', 0, m.start()) + 1)
            for m in ENTRY.finditer(src)]


def c_literal(text):
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'


def main():
    texts = source_texts()
    entries = table_entries()
    errors = []

    seen = {}
    for en, cs, line in entries:
        if en in seen:
            errors.append('lang_cs.cpp:%d: duplicate entry %r (first at line %d)' % (line, en, seen[en]))
        seen[en] = line
        if en not in texts:
            errors.append('lang_cs.cpp:%d: not used in the sources: %r' % (line, en))
        if PRINTF.findall(en) != PRINTF.findall(cs):
            errors.append('lang_cs.cpp:%d: printf conversions differ: %r -> %r' % (line, en, cs))
        bad = sorted(set(c for c in cs if ord(c) > 127 and c not in CZECH))
        if bad:
            errors.append('lang_cs.cpp:%d: characters the font can\'t draw: %s' % (line, ''.join(bad)))

    missing = [(t, where) for t, where in texts.items() if t not in seen and t not in SAME]
    for t, where in missing:
        errors.append('%s: no Czech translation: %r' % (where, t))

    if '--missing' in sys.argv:
        for t, where in missing:
            print('  { %s,\n    %s },  // %s' % (c_literal(t), c_literal(t), where))
        return 0

    for e in errors:
        print(e)
    print('%d marked texts, %d translations, %d problems' % (len(texts), len(entries), len(errors)))
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
