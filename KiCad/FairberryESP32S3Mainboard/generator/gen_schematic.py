import sys, os, re, uuid, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sexp import parse_file, find_direct, find_all

def new_uuid():
    return str(uuid.uuid4())

def extract_raw_symbol_block(libfile, symname):
    """Extract the raw text of a top-level symbol block (including all its
    sub-units) from a KiCad symbol library file, by finding matching
    top-level (symbol "NAME" ... ) boundaries via the already-parsed tree
    (for locating start) then counting parens in the raw text for the end."""
    with open(libfile) as f:
        content = f.read()
    # Must match the exact top-level declaration "(symbol "NAME"", not any
    # occurrence of the quoted name elsewhere (e.g. inside another symbol's
    # property value, which a bare substring search would find first).
    start = content.find(f'(symbol "{symname}"')
    if start == -1:
        raise ValueError(f"{symname} not found in {libfile}")
    # Walk forward counting paren depth to find the matching close paren.
    depth = 0
    i = start
    n = len(content)
    in_str = False
    while i < n:
        c = content[i]
        if c == '"' and content[i-1] != '\\':
            in_str = not in_str
        elif not in_str:
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    return content[start:i+1]
        i += 1
    raise ValueError("unbalanced parens")

def get_pin_defs(libfile, symname):
    tree = parse_file(libfile)
    symbols = find_direct(tree, 'symbol')
    target = None
    for s in symbols:
        if s[1] == symname:
            target = s
            break
    if not target:
        raise ValueError(f"{symname} not found")
    pins = find_all(target, 'pin')
    result = []
    for p in pins:
        name_entry = find_direct(p, 'name')
        num_entry = find_direct(p, 'number')
        at_entry = find_direct(p, 'at')
        name = name_entry[0][1] if name_entry else '?'
        num = num_entry[0][1] if num_entry else '?'
        x, y, rot = float(at_entry[0][1]), float(at_entry[0][2]), float(at_entry[0][3]) if len(at_entry[0]) > 3 else 0
        result.append({'num': num, 'name': name, 'x': x, 'y': y, 'rot': rot, 'etype': p[1]})
    fp = None
    for prop in find_direct(target, 'property'):
        if prop[1] == 'Footprint':
            fp = prop[2]
    return result, fp

print("Module loaded OK")
